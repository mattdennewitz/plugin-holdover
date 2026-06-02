#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace holdover {

// Flat dark theme: minimal rotary knobs, switch/LED toggles, thin readouts.
class HoldoverLookAndFeel : public juce::LookAndFeel_V4 {
public:
    HoldoverLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float pos, float startAngle, float endAngle,
                          juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool highlighted, bool down) override;

    static constexpr juce::uint32 kBg     = 0xff141416;
    static constexpr juce::uint32 kPanel  = 0xff1c1c20;
    static constexpr juce::uint32 kAccent = 0xff5ad1c4;
    static constexpr juce::uint32 kText   = 0xffd8d8dc;
    static constexpr juce::uint32 kMute   = 0xffd65b5b;
    static constexpr juce::uint32 kOffDot = 0xff3a3a40;
    static constexpr juce::uint32 kTrack   = 0xff26262c;
    static constexpr juce::uint32 kTextDim = 0xff9a9aa2;
};

} // namespace holdover
