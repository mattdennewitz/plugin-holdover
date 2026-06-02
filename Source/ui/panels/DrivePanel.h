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
        sat_.attach(*this, s, "satEngage", "SAT", ToggleStyle::Led);
        hex_.attach(*this, s, "hexEngage", "HEX", ToggleStyle::Led);
        curve_.attach(*this, s, "curve", "CURVE", ToggleStyle::Led);
    }
    void resized() override {
        auto a = contentArea();
        auto row = a.removeFromTop(ui::kCellH);
        auto driveCell = row.removeFromLeft(ui::kCellW); row.removeFromLeft(ui::kCellGap);
        auto masCell   = row.removeFromLeft(ui::kCellW); row.removeFromLeft(ui::kCellGap * 2);
        drive_.layout(driveCell);
        mas_.layout(masCell);
        // Three LED buttons stacked, vertically centred in the remaining width.
        constexpr int gap = 6; // vertical gap between the stacked LED buttons
        auto stack = row.withSizeKeepingCentre(juce::jmin(row.getWidth(), 120),
                                               3 * ui::kToggleH + 2 * gap);
        sat_.layout(stack.removeFromTop(ui::kToggleH));   stack.removeFromTop(gap);
        hex_.layout(stack.removeFromTop(ui::kToggleH));   stack.removeFromTop(gap);
        curve_.layout(stack.removeFromTop(ui::kToggleH));
    }
private:
    AttachedKnob drive_;
    AttachedChoice mas_;
    AttachedToggle sat_, hex_, curve_;
};

} // namespace holdover
