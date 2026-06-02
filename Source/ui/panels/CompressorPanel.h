#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class CompressorPanel : public Panel {
public:
    explicit CompressorPanel(APVTS& s) : Panel("COMPRESSOR") {
        engage_.attach(*this, s, "compEngage", "COMP IN", ToggleStyle::Switch);
        rms_.attach(*this, s, "rmsMode", "RMS", ToggleStyle::Led);
        scFilt_.attach(*this, s, "scFilter", "SC FILTER", ToggleStyle::Led);
        thr_.attach(*this, s, "threshold", "Threshold");
        beh_.attach(*this, s, "behavior", "Behavior");
        mak_.attach(*this, s, "makeup", "Makeup");
        atk_.attach(*this, s, "attack", "Attack", params::timeOptions);
        rel_.attach(*this, s, "release", "Release", params::timeOptions);
        sc_.attach(*this, s, "scSource", "SC Src", params::scSourceOptions);
    }
    void resized() override {
        auto a = contentArea();
        auto tog = a.removeFromTop(ui::kToggleH);
        // Widths sized to label text: the pill switch is wider than the LED buttons.
        engage_.layout(tog.removeFromLeft(120)); tog.removeFromLeft(ui::kCellGap);
        rms_.layout(tog.removeFromLeft(70));     tog.removeFromLeft(ui::kCellGap);
        scFilt_.layout(tog.removeFromLeft(108));
        a.removeFromTop(ui::kRowGap);
        auto c = ui::cells(a.removeFromTop(ui::kCellH), 6);
        thr_.layout(c[0]); beh_.layout(c[1]); mak_.layout(c[2]);
        atk_.layout(c[3]); rel_.layout(c[4]); sc_.layout(c[5]);
    }
private:
    AttachedToggle engage_, rms_, scFilt_;
    AttachedKnob thr_, beh_, mak_;
    AttachedChoice atk_, rel_, sc_;
};

} // namespace holdover
