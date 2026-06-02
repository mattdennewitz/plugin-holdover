#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class MatrixPanel : public Panel {
public:
    explicit MatrixPanel(APVTS& s) : Panel("MATRIX / OUTPUT") {
        dryEq_.attach(*this, s, "dryEqFeedLevel", "Dry/EQ");
        comp_.attach(*this, s, "compFeedLevel", "Comp");
        sat_.attach(*this, s, "satFeedLevel", "Sat");
        out_.attach(*this, s, "output", "Output");
        dryEqMute_.attach(*this, s, "dryEqMute", "Mute");
        compMute_.attach(*this, s, "compMute", "Mute");
        satMute_.attach(*this, s, "satMute", "Mute");
        dryEqSrc_.attach(*this, s, "dryEqSource", "Dry Src", params::dryEqSrcOptions);
        ceiling_.attach(*this, s, "ceiling", "Ceiling");
    }
    void resized() override {
        auto area = contentArea();
        auto c = columns(area, 4);
        auto feedCol = [](juce::Rectangle<int> col, AttachedKnob& k, AttachedToggle& m) {
            auto mute = col.removeFromBottom(22);
            k.layout(col); m.layout(mute);
        };
        feedCol(c[0], dryEq_, dryEqMute_);
        feedCol(c[1], comp_, compMute_);
        feedCol(c[2], sat_, satMute_);
        auto last = c[3];
        out_.layout(last.removeFromTop(last.getHeight() / 2));
        dryEqSrc_.layout(last.removeFromTop(40));
        ceiling_.layout(last.removeFromTop(24));
    }
private:
    AttachedKnob dryEq_, comp_, sat_, out_;
    AttachedToggle dryEqMute_, compMute_, satMute_, ceiling_;
    AttachedChoice dryEqSrc_;
};

} // namespace holdover
