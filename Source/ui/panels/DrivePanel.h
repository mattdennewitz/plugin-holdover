#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class DrivePanel : public Panel {
public:
    explicit DrivePanel(APVTS& s) : Panel("DRIVE") {
        drive_.attach(*this, s, "drive", "Drive");
        mas_.attach(*this, s, "masMode", "MAS", params::masOptions);
        sat_.attach(*this, s, "satEngage", "SAT");
        hex_.attach(*this, s, "hexEngage", "HEX");
        curve_.attach(*this, s, "curve", "Curve");
    }
    void resized() override {
        auto area = contentArea();
        auto c = columns(area, 3);
        drive_.layout(c[0]);
        mas_.layout(c[1]);
        auto toggles = c[2];
        const int h = toggles.getHeight() / 3;
        sat_.layout(toggles.removeFromTop(h).reduced(0, 2));
        hex_.layout(toggles.removeFromTop(h).reduced(0, 2));
        curve_.layout(toggles.removeFromTop(h).reduced(0, 2));
    }
private:
    AttachedKnob drive_;
    AttachedChoice mas_;
    AttachedToggle sat_, hex_, curve_;
};

} // namespace holdover
