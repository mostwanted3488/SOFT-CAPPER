cmake_minimum_required(VERSION 3.22)

project(SOFT_CAPPER VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

set(SC_FORMATS VST3)
if(APPLE)
    list(APPEND SC_FORMATS AU)
endif()

juce_add_plugin(SoftCapper
    COMPANY_NAME                "BUYORBEALOSER"
    BUNDLE_ID                   "com.buyorbealoser.softcapper"
    PLUGIN_MANUFACTURER_CODE    Bobl
    PLUGIN_CODE                 Scap
    FORMATS                     ${SC_FORMATS}
    PRODUCT_NAME                "SOFT CAPPER"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Distortion Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect
    NEEDS_WEBVIEW2              TRUE
)

juce_add_binary_data(SoftCapperUI
    HEADER_NAME UIData.h
    NAMESPACE   UIData
    SOURCES
        ui/index.html
        ui/fonts/Anton-Regular.ttf
        ui/fonts/PermanentMarker-Regular.ttf
        ui/fonts/JetBrainsMono.ttf
)

target_sources(SoftCapper PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(SoftCapper PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

target_link_libraries(SoftCapper
    PRIVATE
        SoftCapperUI
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
