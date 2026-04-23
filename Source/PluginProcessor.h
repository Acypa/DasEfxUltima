#pragma once

#include <JuceHeader.h>
#include "ZDFFilter.h"

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

    const juce::String getName() const override { return "DasEfxUltima"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts;

    // Объекты фильтров из твоего нового ZDFFilter.h
    ZDFFilter hpf, bell, lpf;

    // Параметры динамики (Atomic для безопасности потоков)
    std::atomic<float> currentGateLevel { 1.0f };
    std::atomic<float> envelope { 0.0f };
    
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DasEfxUltimaProcessor)
};
