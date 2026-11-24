cmake_minimum_required(VERSION 3.22)
project(AmpSuite VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# NOTE: all sources are in the same folder as this file (flat layout).
function(add_amp target product code define)
    juce_add_plugin(${target}
        COMPANY_NAME "Onochie"
        PLUGIN_MANUFACTURER_CODE Onoc
        PLUGIN_CODE ${code}
        FORMATS VST3
        PRODUCT_NAME "${product}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)
    target_sources(${target} PRIVATE AmpProcessor.cpp AmpProcessor.h AmpEditor.h AmpCore.h)
    target_compile_definitions(${target} PRIVATE
        ${define}
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
    target_link_libraries(${target} PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endfunction()

add_amp(BlackfaceFender  "Blackface Fender"              Bfen AMP_FENDER)
add_amp(TwoRockClassic   "Two Rock Classic Reverb Signature"   Tr10 AMP_TWOROCK)
