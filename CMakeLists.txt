cmake_minimum_required(VERSION 3.22)
project(SmoothRide VERSION 0.9.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Clone JUCE next to this CMakeLists.txt: git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found. Clone JUCE into the JUCE subdirectory (see README.md).")
endif()
add_subdirectory(JUCE)
juce_add_plugin(SmoothRide
    COMPANY_NAME "SmoothRideAudio"
    BUNDLE_ID "audio.smoothride.plugin"
    PLUGIN_MANUFACTURER_CODE SmRd
    PLUGIN_CODE Srde
    FORMATS VST3
    PRODUCT_NAME "SmoothRide"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(SmoothRide)
target_sources(SmoothRide PRIVATE Source/PluginProcessor.cpp Source/PluginProcessor.h Source/GainRider.h Source/PerformanceTelemetry.h Source/ParameterCache.h Source/PresetWorkflowPolicy.h Source/PluginEditor.cpp Source/PluginEditor.h)
target_compile_definitions(SmoothRide PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SmoothRide PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_recommended_config_flags juce::juce_recommended_warning_flags juce::juce_recommended_lto_flags)

# Standalone, dependency-free DSP regression tests. Enabled in CI via CTest.
include(CTest)
if(BUILD_TESTING)
    add_executable(SmoothRideDSPTest tests/dsp_test.cpp)
    add_executable(SmoothRidePerformanceTest tests/performance_test.cpp)
    add_executable(SmoothRideParameterCacheTest tests/parameter_cache_test.cpp)
    add_executable(SmoothRidePresetWorkflowTest tests/preset_workflow_test.cpp)
    add_test(NAME PresetWorkflowPolicy COMMAND SmoothRidePresetWorkflowTest)
    add_test(NAME ParameterCacheRegression COMMAND SmoothRideParameterCacheTest)
    add_test(NAME DSPRegression COMMAND SmoothRideDSPTest)
    add_test(NAME PerformanceTelemetry COMMAND SmoothRidePerformanceTest)
endif()
