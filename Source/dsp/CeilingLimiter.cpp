#include "CeilingLimiter.h"
#include <juce_core/juce_core.h>
#include <cmath>

namespace holdover {

void CeilingLimiter::prepare(double sampleRate) noexcept {
    coeff_ = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi
                             * 6000.0f / (float) sampleRate);
    reset();
}

float CeilingLimiter::processSample(float x) noexcept {
    if (!engaged_) return x;
    const float clipped = juce::jlimit(-kCeiling, kCeiling, x);
    z_ += coeff_ * (clipped - z_);
    return z_;
}

} // namespace holdover
