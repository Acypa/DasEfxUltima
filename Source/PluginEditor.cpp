#include "PluginProcessor.h"
#include "PluginEditor.h"

DasEfxUltimaEditor::DasEfxUltimaEditor (DasEfxUltimaProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. Настраиваем слайдеры (внешний вид)
    auto setupSlider = [this](juce::Slider& s, juce::String suffix) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(s);
    };

    setupSlider(driveSlider, " Drive");
    setupSlider(intensitySlider, " Inst");
    setupSlider(cfoFreqSlider, " Freq");

    addAndMakeVisible(autoButton);
    autoButton.setButtonText("AUTO");
    
    addAndMakeVisible(manualButton);
    manualButton.setButtonText("MANUAL");

    // 2. Привязываем всё к процессору (APVTS)
    driveAttach = std::make_unique<Attachment>(audioProcessor.apvts, "drive", driveSlider);
    intensityAttach = std::make_unique<Attachment>(audioProcessor.apvts, "intensity", intensitySlider);
    cfoAttach = std::make_unique<Attachment>(audioProcessor.apvts, "cfoFreq", cfoFreqSlider);
    autoAttach = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "autoMode", autoButton);
    manualAttach = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "manualMode", manualButton);

    // Цвета сегментов для "инзестеционального благоденствия"
    mudBand.colour = juce::Colours::indianred;
    boxyBand.colour = juce::Colours::orange;
    harshBand.colour = juce::Colours::cyan;

    setSize (500, 300);
    startTimerHz(30); // Обновляем экран 30 раз в секунду
}

DasEfxUltimaEditor::~DasEfxUltimaEditor() { stopTimer(); }

void DasEfxUltimaEditor::timerCallback()
{
    // По сути, забираем уровень из процессора для анимации
    float level = audioProcessor.getEnvelopeValue(); // Нужно добавить этот геттер в Processor
    
    // Обновляем активность сегментов (плавное падение)
    mudBand.activity = juce::jmax(mudBand.activity * 0.9f, level * 0.5f);
    boxyBand.activity = juce::jmax(boxyBand.activity * 0.9f, level * 1.2f);
    harshBand.activity = juce::jmax(harshBand.activity * 0.9f, level * 0.8f);

    repaint();
}

void DasEfxUltimaEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xFF1A1A1A)); // Темный фон 90-х

    // Рисуем наши мультибенд-сегменты
    auto drawBand = [&](BandVisualizer& b, juce::String label) {
        g.setColour(b.colour.withAlpha(0.1f + b.activity));
        g.fillRect(b.area);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRect(b.area, 1);
        g.drawText(label, b.area.withHeight(20), juce::Justification::centred);
    };

    drawBand(mudBand, "MUD");
    drawBand(boxyBand, "BOXY");
    drawBand(harshBand, "HARSH");
}

void DasEfxUltimaEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto topArea = area.removeFromTop(150);
    
    // Распределяем сегменты по центру
    int bandWidth = topArea.getWidth() / 3;
    mudBand.area = topArea.removeFromLeft(bandWidth).reduced(5);
    boxyBand.area = topArea.removeFromLeft(bandWidth).reduced(5);
    harshBand.area = topArea.reduced(5);

    // Слайдеры снизу
    auto bottomArea = area;
    driveSlider.setBounds(bottomArea.removeFromLeft(80));
    intensitySlider.setBounds(bottomArea.removeFromRight(80));
    cfoFreqSlider.setBounds(bottomArea.withSize(100, 100).withCentre(bottomArea.getCentre()));
    
    autoButton.setBounds(bottomArea.getX(), bottomArea.getY(), 60, 30);
    manualButton.setBounds(bottomArea.getRight() - 60, bottomArea.getY(), 60, 30);
}
