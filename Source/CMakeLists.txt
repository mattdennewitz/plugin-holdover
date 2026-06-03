# --- DSP core library (headless-testable; no AudioProcessor coupling) ---
add_library(HoldoverDSP STATIC
    Parameters.cpp
    dsp/FilterBlock.cpp
    dsp/EqBlock.cpp
    dsp/Compressor.cpp
    dsp/DriveBlock.cpp
    dsp/CeilingLimiter.cpp
    dsp/ChannelStrip.cpp
    # dsp/*.cpp added by later tasks
    presets/FactoryPresets.cpp
    presets/PresetManager.cpp
)
target_include_directories(HoldoverDSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(HoldoverDSP PUBLIC cxx_std_20)
target_link_libraries(HoldoverDSP PUBLIC
    juce::juce_dsp juce::juce_audio_basics juce::juce_audio_processors juce::juce_core)
target_compile_definitions(HoldoverDSP PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODULE_AVAILABLE_juce_dsp=1)

# --- Plugin target ---
juce_add_plugin(Holdover
    PRODUCT_NAME "Holdover"
    COMPANY_NAME "Holdover Audio"
    BUNDLE_ID com.holdoveraudio.holdover
    PLUGIN_MANUFACTURER_CODE Hldv
    PLUGIN_CODE Hldv
    FORMATS VST3 AU
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(Holdover PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    ui/HoldoverLookAndFeel.cpp
    ui/LedMeter.cpp
    ui/PresetBar.cpp)

target_compile_features(Holdover PRIVATE cxx_std_20)
target_link_libraries(Holdover PRIVATE
    HoldoverDSP
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
target_compile_definitions(Holdover PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
