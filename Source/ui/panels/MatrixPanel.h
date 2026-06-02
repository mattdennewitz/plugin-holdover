#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class MatrixPanel : public Panel {
public:
    explicit MatrixPanel(APVTS& s) : Panel("MATRIX / OUTPUT") {
        dryEq_.attach(*this, s, "dryEqFeedLevel", "Dry / EQ");
        comp_.attach(*this, s, "compFeedLevel", "Comp");
        sat_.attach(*this, s, "satFeedLevel", "Sat");
        dryEqSrc_.attach(*this, s, "dryEqSource", "Dry Src", params::dryEqSrcOptions);
        out_.attach(*this, s, "output", "Output");
        dryEqMute_.attach(*this, s, "dryEqMute", "MUTE", ToggleStyle::Mute);
        compMute_.attach(*this, s, "compMute", "MUTE", ToggleStyle::Mute);
        satMute_.attach(*this, s, "satMute", "MUTE", ToggleStyle::Mute);
        ceiling_.attach(*this, s, "ceiling", "CEILING", ToggleStyle::Led);
    }
    void resized() override {
        auto a = contentArea();
        auto c = ui::cells(a.removeFromTop(ui::kCellH), 5);
        dryEq_.layout(c[0]); comp_.layout(c[1]); sat_.layout(c[2]);
        dryEqSrc_.layout(c[3]); out_.layout(c[4]);
        a.removeFromTop(ui::kRowGap);
        auto tog = a.removeFromTop(ui::kToggleH);
        auto m = ui::cells(tog, 5); // align with the 5-cell knob row above
        dryEqMute_.layout(m[0]); compMute_.layout(m[1]); satMute_.layout(m[2]);
        // m[3] (under Dry Src) intentionally empty; CEILING sits under Output.
        ceiling_.layout(m[4]);
    }
private:
    AttachedKnob dryEq_, comp_, sat_, out_;
    AttachedToggle dryEqMute_, compMute_, satMute_, ceiling_;
    AttachedChoice dryEqSrc_;
};

} // namespace holdover
