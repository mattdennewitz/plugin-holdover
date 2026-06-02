#pragma once
#include <juce_core/juce_core.h>

namespace holdover {

// "Power-rail" ceiling: hard-clipper into a fast-release one-pole low-pass, so it
// grinds the waveform toward a near-static distorted state when pushed. NOT a
// transparent lookahead limiter. Must live inside the oversampled region.
class CeilingLimiter {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept { z_ = 0.0f; }
    void setEngaged(bool on) noexcept { engaged_ = on; }
    float processSample(float x) noexcept;
private:
    static constexpr float kCeiling = 0.98f;
    bool engaged_ = false;
    float coeff_ = 0.5f, z_ = 0.0f;
};

} // namespace holdover
