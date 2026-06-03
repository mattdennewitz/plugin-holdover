CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    GIT_TAG v3.5.2)

add_executable(HoldoverTests
    test_main.cpp
    test_scales.cpp
    test_parameters.cpp
    test_filter.cpp
    test_eq.cpp
    test_compressor.cpp
    test_drive.cpp
    test_inputstage.cpp
    test_matrix.cpp
    test_ceiling.cpp
    test_output.cpp
    test_channelstrip.cpp
    test_presets.cpp
    test_presetmanager.cpp
    test_hardening.cpp)

target_link_libraries(HoldoverTests PRIVATE HoldoverDSP Catch2::Catch2WithMain)
target_compile_features(HoldoverTests PRIVATE cxx_std_20)

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(HoldoverTests)

# --- Editor tests (need the GUI; kept separate from the headless DSP suite) ---
add_executable(HoldoverEditorTests
    test_editor_size.cpp
    test_processor_presets.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/HoldoverLookAndFeel.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/LedMeter.cpp)
target_link_libraries(HoldoverEditorTests PRIVATE
    HoldoverDSP juce::juce_audio_utils Catch2::Catch2WithMain)
target_compile_features(HoldoverEditorTests PRIVATE cxx_std_20)
target_compile_definitions(HoldoverEditorTests PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODULE_AVAILABLE_juce_dsp=1
    JUCE_STANDALONE_APPLICATION=1)
catch_discover_tests(HoldoverEditorTests)
