#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../HoldoverLookAndFeel.h"
#include "../Layout.h"

namespace holdover {

class Panel : public juce::Component {
public:
    explicit Panel(juce::String title) : title_(std::move(title)) {}
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour(HoldoverLookAndFeel::kPanel));
        g.fillRoundedRectangle(b, 6.0f);

        auto titleRow = getLocalBounds().removeFromTop(ui::kPanelTitleH).reduced(ui::kPanelPadX, 0);
        g.setColour(juce::Colour(HoldoverLookAndFeel::kText));
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(title_, titleRow, juce::Justification::centredLeft);

        // Hairline under the title.
        g.setColour(juce::Colour(HoldoverLookAndFeel::kTrack));
        g.fillRect(ui::kPanelPadX, ui::kPanelTitleH - 1,
                   getWidth() - 2 * ui::kPanelPadX, 1);
    }
    // Inner content rectangle: below the title, padded.
    juce::Rectangle<int> contentArea() const {
        auto a = getLocalBounds().reduced(ui::kPanelPadX, 0);
        a.removeFromTop(ui::kPanelTitleH + ui::kPanelPadTop);
        a.removeFromBottom(ui::kPanelPadBot);
        return a;
    }
    // Retained until Content::resized() is rewritten in a later task (it still calls this).
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
