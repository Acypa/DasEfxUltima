#pragma once
#include <JuceHeader.h>
#define JucePlugin_Build_VST 0

// ================= ZDF FILTER =================
class ZDFFilter {
private:
    float envelope = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float currentGateLevel = 1.0f;
public:
    enum Type { LowPass, HighPass, Bell };

    void setParams(float freq, float res, float sampleRate, Type t, float gainDB = 0.0f)
    {
        float g = std::tan(juce::MathConstants<float>::pi * freq / sampleRate);
        float k = 1.0f / res;
        float gainLinear = std::pow(10.0f, gainDB / 20.0f);

        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;

        type = t;
        this->g = g;
        this->k = k;
        this->gain = gainLinear;
    }

    float process(float x)
    {
        float v3 = x - s2;
        float v1 = a1 * s1 + a2 * v3;
        float v2 = s2 + a2 * s1 + a3 * v3;

        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;

        if (type == LowPass)  return v2;
        if (type == HighPass) return x - k * v1 - v2;
        if (type == Bell)     return x + k * (gain - 1.0f) * v1;

        return x;
    }

private:
    float s1 = 0.0f, s2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float g = 0.0f, k = 0.0f, gain = 1.0f;
    Type type = LowPass;
};

// ================= PROCESSOR =================
class DasEfxUltimaProcessor : public juce::AudioProcessor
{
public:
    DasEfxUltimaProcessor();
    ~DasEfxUltimaProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Das EFX 1993 Studio"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    float getLevel() const { return rmsLevel.get(); }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    ZDFFilter hpf, lpf, bell;

    // Envelope auto-gain
    float envelope = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float currentGateLevel = 1.0f;

    juce::Atomic<float> rmsLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DasEfxUltimaProcessor)
};
