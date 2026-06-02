#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class EqPanel : public Panel {
public:
    explicit EqPanel(APVTS& s) : Panel("EQ") {
        engage_.attach(*this, s, "eqEngage", "EQ In");
        bassDb_.attach(*this, s, "bassDb", "Bass");
        bassFreq_.attach(*this, s, "bassFreq", "Bass Hz", params::bassFreqOptions);
        presDb_.attach(*this, s, "presenceDb", "Presence");
        presFreq_.attach(*this, s, "presenceFreq", "Pres Hz");
        trebDb_.attach(*this, s, "trebleDb", "Treble");
        trebFreq_.attach(*this, s, "trebleFreq", "Treble Hz", params::trebleFreqOptions);
    }
    void resized() override {
        auto area = contentArea();
        engage_.layout(area.removeFromTop(24));
        area.removeFromTop(6);
        // Six columns: bass gain | bass freq | presence gain | presence freq | treble gain | treble freq
        auto c = columns(area, 6);
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
