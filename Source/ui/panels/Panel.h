#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "../HoldoverLookAndFeel.h"

namespace holdover {

class Panel : public juce::Component {
public:
    explicit Panel(juce::String title) : title_(std::move(title)) {}
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(HoldoverLookAndFeel::kPanel));
        g.fillRoundedRectangle(b, 5.0f);
        g.setColour(juce::Colour(HoldoverLookAndFeel::kText));
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(title_, getLocalBounds().removeFromTop(20).reduced(8, 2),
                   juce::Justification::centredLeft);
    }
    juce::Rectangle<int> contentArea() const { return getLocalBounds().withTrimmedTop(22).reduced(8); }
    static std::vector<juce::Rectangle<int>> columns(juce::Rectangle<int> a, int n, int gap = 6) {
        std::vector<juce::Rectangle<int>> out;
        if (n <= 0) return out;
        const int w = (a.getWidth() - gap * (n - 1)) / n;
        for (int i = 0; i < n; ++i) { out.push_back(a.removeFromLeft(w)); if (i < n - 1) a.removeFromLeft(gap); }
        return out;
    }
private:
    juce::String title_;
};

} // namespace holdover
