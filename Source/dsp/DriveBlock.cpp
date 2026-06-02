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
    reset();
}

void DriveBlock::reset() noexcept {
    pre_.reset(); de_.reset(); dcX1_ = dcY1_ = 0.0f;
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
    character_ = juce::jlimit(0.0f, 1.0f, amount01);
    // A small DC offset pushed into the shaper biases it off-center, generating
    // 2nd-harmonic "growl". Opposite sign per channel de-correlates L/R. Ear-tuned.
    constexpr float kBiasMax = 0.1f;
    bias_ = chSign * kBiasMax * character_;
}

float DriveBlock::masStage(float x) const noexcept {
    if (mas_ == 1) {                      // 2nd-emphasis: asymmetric, even harmonics
        // Normalize input to avoid hard-clipping into a square wave (which produces
        // dominant odd harmonics). tanh(x*0.25) keeps us in the soft-saturation region
        // even at high preGain, then the x^2 term biases the waveform asymmetrically
        // to generate 2nd harmonic content.
        const float xn = std::tanh(x * 0.25f);
        return xn + 0.45f * (xn * xn);
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
    // Class-A asymmetric bias: only meaningful when a shaper follows. bias_ is 0 when
    // character is 0, so the stages-off identity path below is unchanged.
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
