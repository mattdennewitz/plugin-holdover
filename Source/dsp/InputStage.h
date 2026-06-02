#pragma once
#include "Scales.h"

namespace holdover {

// GAIN L / GAIN R per-channel trims followed by the master INPUT fader.
class InputStage {
public:
    void setGains(float gainLpos, float gainRpos, float inputPos) noexcept {
        trimL_ = scales::trimGain(gainLpos);
        trimR_ = scales::trimGain(gainRpos);
        input_ = scales::faderGain(inputPos);
    }
    void processStereo(float& l, float& r) const noexcept {
        l *= trimL_ * input_;
        r *= trimR_ * input_;
    }
private:
    float trimL_ = 1.0f, trimR_ = 1.0f, input_ = 1.0f;
};

} // namespace holdover
