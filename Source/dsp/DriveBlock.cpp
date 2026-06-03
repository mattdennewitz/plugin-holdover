#include "DriveBlock.h"
#include <juce_core/juce_core.h>
#include "Scales.h"
#include <cmath>

namespace holdover {

DriveBlock::Shelf DriveBlock::makeEmphasis(double sr, float shelfDb, float cornerHz) noexcept {
    const float wc = std::tan(juce::MathConstants<float>::pi * cornerHz / (float) sr);
    const float g  = std::pow(10.0f, shelfDb / 20.0f);
    const float n  = 1.0f / (wc + 1.0f);
    Shelf s;
    s.b0 = (wc + g) * n;
    s.b1 = (wc - g) * n;
    s.a1 = (wc - 1.0f) * n;
    return s;
}

DriveBlock::Shelf DriveBlock::invert(const Shelf& s) noexcept {
    Shelf inv;
    inv.b0 = 1.0f / s.b0;
    inv.b1 = s.a1 / s.b0;
    inv.a1 = s.b1 / s.b0;
    return inv;
}

void DriveBlock::prepare(double sampleRate) noexcept {
    sr_ = sampleRate;
    pre_ = makeEmphasis(sr_, 6.0f, 1500.0f);
    de_  = invert(pre_);
    // ~5 ms one-pole smoothing for the bias so Character sweeps don't zipper. sr_ is the
    // oversampled rate, so the time constant is in oversampled samples.
    biasCoeff_ = std::exp(-1.0f / (0.005f * (float) sr_));
    reset();
}

void DriveBlock::reset() noexcept {
    pre_.reset(); de_.reset(); dcX1_ = dcY1_ = 0.0f;
    bias_ = biasNominal_ * juce::jmin(1.0f, preGain_); // start settled, no ramp-in after reset
}

void DriveBlock::setDrive(float pos) noexcept {
    preGain_ = scales::drivePreGain(pos);
    outComp_ = 1.0f / std::sqrt(juce::jmax(1.0e-3f, preGain_));
}
void DriveBlock::setMas(int mode) noexcept { mas_ = mode; }
void DriveBlock::setSat(bool on) noexcept { sat_ = on; }
void DriveBlock::setHex(bool on) noexcept { hex_ = on; }
void DriveBlock::setCurve(bool on) noexcept { curve_ = on; }

void DriveBlock::setCharacter(float amount01, float chSign) noexcept {
    const float amt = juce::jlimit(0.0f, 1.0f, amount01);
    // A DC offset pushed into the shaper biases it off-center, generating 2nd-harmonic
    // "growl". A shared in-phase base (0.85) keeps that even-harmonic warmth alive on a
    // mono fold; a small opposite-sign trim (0.15) de-correlates L/R for width without the
    // full antiphase cancellation a pure +/- bias would suffer. processSample scales this by
    // drive and ramps bias_ toward it (anti-zipper).
    constexpr float kBiasMax = 0.28f;
    biasNominal_ = kBiasMax * amt * (0.85f + 0.15f * chSign);
}

float DriveBlock::masStage(float x) const noexcept {
    if (mas_ == 1) {                      // 2nd-emphasis: asymmetric, even harmonics
        // Normalize input to avoid hard-clipping into a square wave (which produces
        // dominant odd harmonics). tanh(x*0.25) keeps us in the soft-saturation region
        // even at high preGain, then the x^2 term biases the waveform asymmetrically
        // to generate 2nd harmonic content.
        const float xn = std::tanh(x * 0.25f);
        return xn + 0.35f * (xn * xn);
    }
    if (mas_ == 2) {                      // 3rd-emphasis: odd cubic soft clip
        const float c = juce::jlimit(-1.0f, 1.0f, x);
        return 1.5f * (c - c * c * c / 3.0f);
    }
    return x;
}

float DriveBlock::satStage(float x) noexcept {
    return std::tanh(1.6f * x) / std::tanh(1.6f);
}

float DriveBlock::hexStage(float x) noexcept {
    return juce::jlimit(-1.0f, 1.0f, 2.5f * x);
}

float DriveBlock::processSample(float x) noexcept {
    float y = x * preGain_;
    if (curve_) y = pre_.process(y);
    // Class-A asymmetric bias: only meaningful when a shaper follows. Scale by drive so the
    // asymmetry tracks how hard the stage is pushed (capped at unity preGain so it doesn't
    // run away in the red), then ramp bias_ toward that target every sample so Character
    // sweeps don't step (which the DC blocker would turn into tearing). bias_ stays 0 when
    // character is 0, so the stages-off identity path below is unchanged.
    const float biasTarget = biasNominal_ * juce::jmin(1.0f, preGain_);
    bias_ = biasCoeff_ * bias_ + (1.0f - biasCoeff_) * biasTarget;
    const bool stagesActive = (mas_ != 0) || sat_ || hex_;
    if (stagesActive) y += bias_;
    y = masStage(y);
    if (sat_) y = satStage(y);
    if (hex_) y = hexStage(y);
    if (curve_) y = de_.process(y);
    y *= outComp_;
    // DC blocker (also removes the bias-induced DC, leaving the even-harmonic content).
    // With all stages off, CURVE pre/de cancel exactly => keep identity (no HPF phase shift).
    if (!stagesActive) return y;
    const float out = y - dcX1_ + 0.9995f * dcY1_;
    dcX1_ = y; dcY1_ = out;
    return out;
}

} // namespace holdover
