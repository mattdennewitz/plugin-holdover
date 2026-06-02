# Placeholder DSP library; sources are added by later tasks.
add_library(HoldoverDSP STATIC dsp/Placeholder.cpp Parameters.cpp)
target_include_directories(HoldoverDSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(HoldoverDSP PUBLIC cxx_std_20)
target_link_libraries(HoldoverDSP PUBLIC
    juce::juce_dsp juce::juce_audio_basics juce::juce_core juce::juce_audio_processors)
target_compile_definitions(HoldoverDSP PUBLIC JUCE_MODULE_AVAILABLE_juce_dsp=1)
