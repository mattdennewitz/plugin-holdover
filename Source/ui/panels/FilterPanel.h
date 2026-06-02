#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"

namespace holdover {

class FilterPanel : public Panel {
public:
    explicit FilterPanel(APVTS& s) : Panel("FILTERS") {
        engage_.attach(*this, s, "filtEngage", "Filter In");
        hpfFreq_.attach(*this, s, "hpfFreq", "HPF");
        hpfPeak_.attach(*this, s, "hpfPeak", "HPF Peak");
        lpfFreq_.attach(*this, s, "lpfFreq", "LPF");
        lpfPeak_.attach(*this, s, "lpfPeak", "LPF Peak");
    }
    void resized() override {
        auto area = contentArea();
        engage_.layout(area.removeFromTop(24));
        area.removeFromTop(6);
        auto c = columns(area, 4);
        hpfFreq_.layout(c[0]); hpfPeak_.layout(c[1]); lpfFreq_.layout(c[2]); lpfPeak_.layout(c[3]);
    }
private:
    AttachedToggle engage_;
    AttachedKnob hpfFreq_, hpfPeak_, lpfFreq_, lpfPeak_;
};

} // namespace holdover
