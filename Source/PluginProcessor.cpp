#include "PluginProcessor.h"

DasEfxUltimaProcessor::DasEfxUltimaProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      currentGateLevel (1.0f)
{
}

DasEfxUltimaProcessor::~DasEfxUltimaProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
DasEfxUltimaProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "monitorMix", "Monitor Mix", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gateThresh", "Gate Threshold", -60.0f, 0.0f, -45.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Analog Drive", 1.0f, 3.0f, 1.4f));

    return { params.begin(), params.end() };
}

void DasEfxUltimaProcessor::prepareToPlay(double sampleRate, int)
{
    hpf.setParams(125.0f, 0.707f, (float)sampleRate, ZDFFilter::HighPass);
    bell.setParams(2850.0f, 1.3f, (float)sampleRate, ZDFFilter::Bell, 5.0f);
    lpf.setParams(14200.0f, 0.85f, (float)sampleRate, ZDFFilter::LowPass);

    const float attackTime = 0.01f;   // 10 ms
    const float releaseTime = 0.15f;  // 150 ms

    attackCoeff  = std::exp(-1.0f / (sampleRate * attackTime));
    releaseCoeff = std::exp(-1.0f / (sampleRate * releaseTime));
}

void DasEfxUltimaProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float monitorMix = *apvts.getRawParameterValue("monitorMix");
    const float gateDB     = *apvts.getRawParameterValue("gateThresh");
    const float drive      = *apvts.getRawParameterValue("drive");

    const float gateThreshold = juce::Decibels::decibelsToGain(gateDB);
    const float targetLevel   = 0.6f;

    float rmsAccum = 0.0f;
    int totalSamples = 0;

    for (int channel = 0; channel < getTotalNumInputChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float cleanSig = channelData[i];
            float x = cleanSig;

           // 1. Умный Гейт со сглаживанием
float gateGain = (std::abs(x) > gateThreshold) ? 1.0f : 0.02f;

// Используем огибающую для плавного закрытия (Release)
if (gateGain < currentGateLevel)
    currentGateLevel = releaseCoeff * (currentGateLevel - gateGain) + gateGain;
else
    currentGateLevel = gateGain; // Открываем мгновенно (Attack)

x *= currentGateLevel;

            if (rectified > envelope)
                envelope = attackCoeff * (envelope - rectified) + rectified;
            else
                envelope = releaseCoeff * (envelope - rectified) + rectified;

            // 3. Auto Gain
            float gain = (envelope > 0.001f) ? (targetLevel / envelope) : 1.0f;
            gain = juce::jlimit(0.5f, 3.0f, gain);

            x *= gain;

            // 4. Saturation
            x = std::tanh(x * drive);

            // 5. ZDF EQ
            x = hpf.process(x);
            x = bell.process(x);
            x = lpf.process(x);

            // 6. 12-bit crunch
            x = std::round(x * 4096.0f) / 4096.0f;

            // 7. Soft Clip
            x = juce::jlimit(-0.98f, 0.98f, x);

            float processed = x * 0.8f;
            float finalOut =
                (processed * (1.0f - monitorMix)) +
                (cleanSig  * monitorMix);

            channelData[i] = finalOut;

            rmsAccum += finalOut * finalOut;
            totalSamples++;
        }
    }

    float rms = std::sqrt(rmsAccum / (float)juce::jmax(1, totalSamples));
    rmsLevel.set(rms);
}

void DasEfxUltimaProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DasEfxUltimaProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorEditor*
DasEfxUltimaProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DasEfxUltimaProcessor();
}
