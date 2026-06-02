#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class CompressorPanel : public Panel {
public:
    explicit CompressorPanel(APVTS& s) : Panel("COMPRESSOR") {
        engage_.attach(*this, s, "compEngage", "Comp In");
        rms_.attach(*this, s, "rmsMode", "RMS");
        scFilt_.attach(*this, s, "scFilter", "SC Filter");
        thr_.attach(*this, s, "threshold", "Threshold");
        beh_.attach(*this, s, "behavior", "Behavior");
        mak_.attach(*this, s, "makeup", "Makeup");
        atk_.attach(*this, s, "attack", "Attack", params::timeOptions);
        rel_.attach(*this, s, "release", "Release", params::timeOptions);
        sc_.attach(*this, s, "scSource", "SC Source", params::scSourceOptions);
    }
    void resized() override {
        auto area = contentArea();
        auto top = area.removeFromTop(24);
        auto tc = columns(top, 3);
        engage_.layout(tc[0]); rms_.layout(tc[1]); scFilt_.layout(tc[2]);
        area.removeFromTop(6);
        auto c = columns(area, 6);
        thr_.layout(c[0]); beh_.layout(c[1]); mak_.layout(c[2]);
        atk_.layout(c[3]); rel_.layout(c[4]); sc_.layout(c[5]);
    }
private:
    AttachedToggle engage_, rms_, scFilt_;
    AttachedKnob thr_, beh_, mak_;
    AttachedChoice atk_, rel_, sc_;
};

} // namespace holdover
