#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"

namespace holdover {

// Header preset bar: [ combo ▾ ] [*] [ < ] [ > ] [ SAVE ]. Drives the processor's
// PresetManager. The modified marker and combo selection are refreshed from the
// editor timer via syncSelection() (message thread only — no APVTS listener, so
// no audio-thread UI hazard).
class PresetBar : public juce::Component {
public:
    explicit PresetBar(HoldoverProcessor&);

    void resized() override;
    void syncSelection();   // called each editor timer tick

private:
    void refreshList();
    void showSaveDialog();

    HoldoverProcessor& processor_;
    juce::ComboBox   presetBox_;
    juce::Label      modLabel_;
    juce::TextButton prevBtn_, nextBtn_, saveBtn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBar)
};

} // namespace holdover
