#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/HoldoverLookAndFeel.h"
#include "ui/LedMeter.h"
#include "ui/panels/InputPanel.h"
#include "ui/panels/FilterPanel.h"
#include "ui/panels/EqPanel.h"
#include "ui/panels/CompressorPanel.h"
#include "ui/panels/DrivePanel.h"
#include "ui/panels/MatrixPanel.h"

namespace holdover {

class HoldoverEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit HoldoverEditor(HoldoverProcessor&);
    ~HoldoverEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    HoldoverProcessor& processor;
    HoldoverLookAndFeel lnf_;

    InputPanel input_;
    FilterPanel filter_;
    EqPanel eq_;
    CompressorPanel comp_;
    DrivePanel drive_;
    MatrixPanel matrix_;

    LedMeter satMeter_ { "SATURATING" };
    LedMeter grMeter_  { "COMPRESSING" };
    juce::Label grReadout_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HoldoverEditor)
};

} // namespace holdover
