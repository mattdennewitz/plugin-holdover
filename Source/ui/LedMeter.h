#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace holdover {

// Vertical LED-ladder meter. `setLevel` takes a 0..1 normalized value; the meter
// lights segments up to that level with a green->amber->red gradient.
class LedMeter : public juce::Component {
public:
    explicit LedMeter(juce::String caption) : caption_(std::move(caption)) {}
    void setLevel(float normalized) {
        const float v = juce::jlimit(0.0f, 1.0f, normalized);
        if (std::abs(v - level_) > 0.001f) { level_ = v; repaint(); }
    }
    void paint(juce::Graphics&) override;
private:
    static constexpr int kSegments = 12;
    juce::String caption_;
    float level_ = 0.0f;
};

} // namespace holdover
