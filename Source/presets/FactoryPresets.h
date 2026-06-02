#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>

namespace holdover::presets {

struct Preset {
    juce::String name;
    std::vector<std::pair<juce::String, float>> values; // (paramID, real value)
};

const std::vector<Preset>& all();

// Apply preset `index` to the APVTS, converting real values to normalized and
// notifying the host. Out-of-range indices are ignored.
void apply(juce::AudioProcessorValueTreeState& apvts, int index);

} // namespace holdover::presets
