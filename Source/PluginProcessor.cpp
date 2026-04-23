#include "PluginProcessor.h"

DasEfxUltimaProcessor::DasEfxUltimaProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Инициализация уровней
    currentGateLevel.store(1.0f);
    envelope.store(0.0f);
}

DasEfxUltimaProcessor::~DasEfxUltimaProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout DasEfxUltimaProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("monitorMix", "Monitor Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gateThresh", "Gate Threshold", -60.0f, 0.0f, -45.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Analog Drive", 1.0f, 3.0f, 1.4f));
    
    // Наши новые "умные" кнопки и ручки
    params.push_back(std::make_unique<juce::AudioParameterBool>("autoMode", "Auto Radar", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("manualMode", "Manual Adjust", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("cfoFreq", "CFO Frequency", 1000.0f, 8000.0f, 2850.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("intensity", "Intensity", 0.0f, 1.0f, 1.0f));

    return { params.begin(), params.end() };
}

void DasEfxUltimaProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    float sr = (float)sampleRate;
    hpf.setParams(125.0f, 0.707f, sr, ZDFFilter::HighPass);
    bell.setParams(2850.0f, 1.3f, sr, ZDFFilter::Bell, 0.0f);
    lpf.setParams(14200.0f, 0.85f, sr, ZDFFilter::LowPass);

    const float attackTime = 0.005f; // 5ms для четких согласных
    const float releaseTime = 0.15f; 

    attackCoeff = std::exp(-1.0f / (sr * attackTime));
    releaseCoeff = std::exp(-1.0f / (sr * releaseTime));
}

void DasEfxUltimaProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Читаем параметры
    const float monitorMix = *apvts.getRawParameterValue("monitorMix");
    const float gateThreshold = juce::Decibels::decibelsToGain((float)*apvts.getRawParameterValue("gateThresh"));
    const float drive = *apvts.getRawParameterValue("drive");
    const bool isAuto = *apvts.getRawParameterValue("autoMode") > 0.5f;
    const bool isManual = *apvts.getRawParameterValue("manualMode") > 0.5f;
    const float manualCfo = *apvts.getRawParameterValue("cfoFreq");
    const float intensity = *apvts.getRawParameterValue("intensity");

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float cleanSig = channelData[i];
            float x = cleanSig;

            // 1. Умный Гейт (плавный, чтобы не "жевать")
            float gateTarget = (fabsf(x) > gateThreshold) ? 1.0f : 0.001f;
            if (gateTarget > currentGateLevel)
                currentGateLevel += attackCoeff * (gateTarget - currentGateLevel);
            else
                currentGateLevel += releaseCoeff * (gateTarget - currentGateLevel);
            
            x *= currentGateLevel;

            // 2. Детектор огибающей для Auto-функций
            float rectified = fabsf(x);
            if (rectified > envelope)
                envelope += attackCoeff * (rectified - envelope);
            else
                envelope += releaseCoeff * (rectified - envelope);

            // 3. Auto Gain (нормализация перед EQ)
            float gain = (envelope > 0.001f) ? (0.6f / envelope) : 1.0f;
            x *= juce::jlimit(0.5f, 2.5f, gain);

            // 4. Динамическая логика CFO (наш "Радар")
            float targetFreq = manualCfo;
            float targetGain = 0.0f;

            if (isAuto) {
                // Автомат ищет яд в районе 3кГц и давит пропорционально энергии
                targetFreq = 2850.0f + (envelope * 500.0f); 
                targetGain = -6.0f * envelope * intensity; 
            }
            
            if (isManual) {
                // Если включен мануал, добавляем ручную коррекцию (суммируем или перекрываем)
                targetGain -= 4.0f * intensity; 
            }

            bell.setParams(targetFreq, 1.3f, (float)getSampleRate(), ZDFFilter::Bell, targetGain);

            // 5. Обработка фильтрами
            x = hpf.process(x);
            x = bell.process(x);
            x = lpf.process(x);

            // 6. Saturation (Analog Character)
            x = std::tanh(x * drive);

            // 7. 12-bit crunch & Final Clip
            x = std::round(x * 4096.0f) / 4096.0f;
            x = juce::jlimit(-0.98f, 0.98f, x);

            // 8. Mix (мониторинг для записи)
            channelData[i] = (x * 0.8f * (1.0f - monitorMix)) + (cleanSig * monitorMix);
        }
    }
}

// Стандартные методы JUCE для сохранения стейта
void DasEfxUltimaProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DasEfxUltimaProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DasEfxUltimaProcessor(); }
