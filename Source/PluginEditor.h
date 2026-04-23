#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class DasEfxUltimaEditor : public juce::AudioProcessorEditor,
                           public juce::Timer // Добавляем таймер для обновления GUI
{
public:
    DasEfxUltimaEditor (DasEfxUltimaProcessor&);
    ~DasEfxUltimaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override; // Здесь будем обновлять наши мультибенд-индикаторы

private:
    DasEfxUltimaProcessor& audioProcessor;

    // Слайдеры и кнопки (наши инструменты)
    juce::Slider driveSlider, intensitySlider, cfoFreqSlider;
    juce::ToggleButton autoButton, manualButton;

    // Аттачменты (связующее звено с APVTS)
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<Attachment> driveAttach, intensityAttach, cfoAttach;
    std::unique_ptr<ButtonAttachment> autoAttach, manualAttach;

    // Секция визуализации (те самые сегменты)
    struct BandVisualizer {
        float activity = 0.0f;
        juce::Rectangle<int> area;
        juce::Colour colour;
    } mudBand, boxyBand, harshBand;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DasEfxUltimaEditor)
};
