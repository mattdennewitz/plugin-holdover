#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace holdover {

class HoldoverEditor : public juce::AudioProcessorEditor {
public:
    explicit HoldoverEditor(HoldoverProcessor&);
    ~HoldoverEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HoldoverProcessor& processor;
    juce::GenericAudioProcessorEditor generic; // replaced by panels in Phase 6
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HoldoverEditor)
};

} // namespace holdover
