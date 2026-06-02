#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace holdover::params {

// Choice option strings (indices are the stored choice values).
inline const juce::StringArray bassFreqOptions   { "50 Hz", "150 Hz", "300 Hz" };
inline const juce::StringArray trebleFreqOptions { "5 kHz", "10 kHz", "15 kHz" };
inline const juce::StringArray timeOptions       { "Fast", "Medium", "Slow" };
inline const juce::StringArray masOptions        { "Off", "2nd", "3rd" };
inline const juce::StringArray scSourceOptions   { "Pre", "Ext", "Post" };
inline const juce::StringArray dryEqSrcOptions   { "Pre", "Post" };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

// All declared parameter IDs, for tests and iteration.
const juce::StringArray& allIDs();

} // namespace holdover::params
