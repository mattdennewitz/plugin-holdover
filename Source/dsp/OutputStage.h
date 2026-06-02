#pragma once
#include "Scales.h"

namespace holdover {

class OutputStage {
public:
    void setLevel(float outputPos) noexcept { gain_ = scales::faderGain(outputPos); }
    void processStereo(float& l, float& r) const noexcept { l *= gain_; r *= gain_; }
private:
    float gain_ = 1.0f;
};

} // namespace holdover
