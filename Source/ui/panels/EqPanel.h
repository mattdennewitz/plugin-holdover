#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class EqPanel : public Panel {
public:
    explicit EqPanel(APVTS& s) : Panel("EQ") {
        engage_.attach(*this, s, "eqEngage", "EQ IN", ToggleStyle::Switch);
        bassDb_.attach(*this, s, "bassDb", "Bass");
        bassFreq_.attach(*this, s, "bassFreq", "Bass Hz", params::bassFreqOptions);
        presDb_.attach(*this, s, "presenceDb", "Presence");
        presFreq_.attach(*this, s, "presenceFreq", "Pres Hz");
        trebDb_.attach(*this, s, "trebleDb", "Treble");
        trebFreq_.attach(*this, s, "trebleFreq", "Treble Hz", params::trebleFreqOptions);
    }
    void resized() override {
        auto a = contentArea();
        engage_.layout(a.removeFromTop(ui::kToggleH).removeFromLeft(110));
        a.removeFromTop(ui::kRowGap);
        auto c = ui::cells(a.removeFromTop(ui::kCellH), 6);
        bassDb_.layout(c[0]); bassFreq_.layout(c[1]);
        presDb_.layout(c[2]); presFreq_.layout(c[3]);
        trebDb_.layout(c[4]); trebFreq_.layout(c[5]);
    }
private:
    AttachedToggle engage_;
    AttachedKnob bassDb_, presDb_, presFreq_, trebDb_;
    AttachedChoice bassFreq_, trebFreq_;
};

} // namespace holdover
