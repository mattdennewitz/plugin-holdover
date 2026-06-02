#pragma once
#include <cmath>

namespace holdover {

// Two cascaded nonlinear TPT state-variable filters (HPF then LPF). Resonance
// (PEAK, 0..10) lowers damping; above ~8 the loop self-oscillates. A tanh inside
// the integrator state self-limits the oscillation amplitude and adds grit when
// pushed. This loop nonlinearity is distinct from DriveBlock saturation.
class FilterBlock {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setHpf(float freqHz, float peak0to10) noexcept;
    void setLpf(float freqHz, float peak0to10) noexcept;

    float processSample(float x) noexcept;

private:
    struct Svf {
        float g = 0.0f, k = 2.0f;
        float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        float ic1 = 0.0f, ic2 = 0.0f;
        void setCoeffs(float gIn, float kIn) noexcept;
        void reset() noexcept { ic1 = ic2 = 0.0f; }
        void process(float x, float& lp, float& hp) noexcept;
    };

    static float dampingFromPeak(float peak) noexcept;
    static float gFromFreq(float fc, double sr) noexcept;

    double sr_ = 48000.0;
    Svf hp_, lp_;
};

} // namespace holdover
