cmake_minimum_required(VERSION 3.15)
set(JucePlugin_Build_VST 0)
set(JucePlugin_Build_VST3 1)
project(DasEfxUltima VERSION 1.0.1)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

set(JUCE_COPY_PLUGIN_AFTER_BUILD OFF)
juce_add_plugin(DasEfxUltima
    COMPANY_NAME "Boss Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Boss
    PLUGIN_CODE EFX9
    FORMATS VST3 AU
    
    JUCE_VST2_CAN_REPLACE_VST3 OFF
    JUCE_VST3_CAN_REPLACE_VST2 OFF
    
    PRODUCT_NAME "DasEfxUltima")   

target_sources(DasEfxUltima PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)

juce_generate_juce_header(DasEfxUltima) 

target_link_libraries(DasEfxUltima
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp)
set_target_properties(DasEfxUltima PROPERTIES JUCE_VST2_CAN_REPLACE_VST3 OFF)
