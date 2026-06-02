#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"

namespace holdover {

class FilterPanel : public Panel {
public:
    explicit FilterPanel(APVTS& s) : Panel("FILTERS") {
        engage_.attach(*this, s, "filtEngage", "FILTER IN", ToggleStyle::Switch);
        hpfFreq_.attach(*this, s, "hpfFreq", "HPF");
        hpfPeak_.attach(*this, s, "hpfPeak", "HPF Peak");
        lpfFreq_.attach(*this, s, "lpfFreq", "LPF");
        lpfPeak_.attach(*this, s, "lpfPeak", "LPF Peak");
    }
    void resized() override {
        auto a = contentArea();
        engage_.layout(a.removeFromTop(ui::kToggleH).removeFromLeft(130));
        a.removeFromTop(ui::kRowGap);
        auto c = ui::cells(a.removeFromTop(ui::kCellH), 4);
        hpfFreq_.layout(c[0]); hpfPeak_.layout(c[1]); lpfFreq_.layout(c[2]); lpfPeak_.layout(c[3]);
    }
private:
    AttachedToggle engage_;
    AttachedKnob hpfFreq_, hpfPeak_, lpfFreq_, lpfPeak_;
};

} // namespace holdover
