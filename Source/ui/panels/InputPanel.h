#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"

namespace holdover {

class InputPanel : public Panel {
public:
    explicit InputPanel(APVTS& s) : Panel("INPUT") {
        gainL_.attach(*this, s, "gainL", "Gain L");
        gainR_.attach(*this, s, "gainR", "Gain R");
        input_.attach(*this, s, "input", "Input");
    }
    void resized() override {
        auto c = columns(contentArea(), 3);
        gainL_.layout(c[0]); gainR_.layout(c[1]); input_.layout(c[2]);
    }
private:
    AttachedKnob gainL_, gainR_, input_;
};

} // namespace holdover
