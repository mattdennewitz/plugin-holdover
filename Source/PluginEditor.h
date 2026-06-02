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

    // Natural design size. The whole UI is laid out once at these dimensions and
    // scaled uniformly to the window, so no control ever squishes or stretches.
    static constexpr int kBaseWidth  = 1020;
    static constexpr int kBaseHeight = 680;

private:
    void timerCallback() override;

    // All controls live here, laid out at the fixed base size. The editor only
    // scales this component; it never repositions individual controls per pixel.
    class Content : public juce::Component {
    public:
        explicit Content(juce::AudioProcessorValueTreeState&);
        void resized() override;
        void updateMeters(float saturation, float gainReductionDb);

    private:
        InputPanel input_;
        FilterPanel filter_;
        EqPanel eq_;
        CompressorPanel comp_;
        DrivePanel drive_;
        MatrixPanel matrix_;

        LedMeter satMeter_ { "SATURATING" };
        LedMeter grMeter_  { "COMPRESSING" };
        juce::Label grReadout_;
    };

    HoldoverProcessor& processor;
    HoldoverLookAndFeel lnf_;
    Content content_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HoldoverEditor)
};

} // namespace holdover
