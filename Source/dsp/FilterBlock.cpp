#include "FilterBlock.h"
#include <juce_core/juce_core.h>

namespace holdover {

static inline float fastTanh(float x) noexcept {
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

float FilterBlock::gFromFreq(float fc, double sr) noexcept {
    fc = juce::jlimit(10.0f, (float) (sr * 0.49), fc);
    return std::tan(juce::MathConstants<float>::pi * fc / (float) sr);
}

float FilterBlock::dampingFromPeak(float peak) noexcept {
    peak = juce::jlimit(0.0f, 10.0f, peak);
    if (peak <= 8.0f)
        return juce::jmap(peak, 0.0f, 8.0f, 2.0f, 0.05f);
    return juce::jmap(peak, 8.0f, 10.0f, 0.05f, -0.08f);
}

void FilterBlock::Svf::setCoeffs(float gIn, float kIn) noexcept {
    g = gIn; k = kIn;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
}

void FilterBlock::Svf::process(float x, float& lp, float& hp) noexcept {
    // Apply tanh on the feedback read of ic1 with a scale that keeps the filter
    // nearly linear for moderate resonance but self-limits at self-oscillation
    // amplitudes (avoids digital runaway while preserving resonant peaks).
    constexpr float kSatScale = 0.1f;
    const float ic1_sat = fastTanh(ic1 * kSatScale) / kSatScale;
    // TPT SVF: the k*v1 damping is re-injected on the next iteration via the integrator states. The soft-clip below is applied to the ic1 feedback read only, intentionally before damping re-injection (this shapes the nonlinear character).
    const float v3 = x - ic2;
    const float v1 = a1 * ic1_sat + a2 * v3;
    const float v2 = ic2 + a2 * ic1_sat + a3 * v3;
    ic1 = 2.0f * v1 - ic1_sat;
    ic2 = 2.0f * v2 - ic2; // ic2 (LP integrator) intentionally NOT saturated; only ic1 (BP integrator) gets the soft-clip.
    lp = v2;
    hp = x - k * v1 - v2;
}

void FilterBlock::prepare(double sampleRate) noexcept { sr_ = sampleRate; reset(); }
// NOTE: ic1/ic2 can drift to denormals during free decay. A juce::ScopedNoDenormals at the processor level (processBlock) covers this; flush here if this filter is ever used outside that wrapper.
void FilterBlock::reset() noexcept { hp_.reset(); lp_.reset(); }

void FilterBlock::setHpf(float freqHz, float peak) noexcept {
    hp_.setCoeffs(gFromFreq(freqHz, sr_), dampingFromPeak(peak));
}
void FilterBlock::setLpf(float freqHz, float peak) noexcept {
    lp_.setCoeffs(gFromFreq(freqHz, sr_), dampingFromPeak(peak));
}

float FilterBlock::processSample(float x) noexcept {
    float lp, hp;
    hp_.process(x, lp, hp);
    lp_.process(hp, lp, hp);
    return lp;
}

} // namespace holdover
