#include "Compressor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include "Scales.h"
#include <cmath>

namespace holdover {

static inline float softSat(float x, float drive) noexcept {
    const float d = 1.0f + drive * 3.0f;
    return std::tanh(d * x) / std::tanh(d);
}

// Asymmetric, odd-dominant VCA waveshaper. `drive` grows with gain reduction, so the
// signal gets thicker the harder the VCA clamps. The bias term adds a touch of 2nd to
// the tanh's odd series for "bite"; subtracting tanh(b) keeps the output centered.
static inline float vcaSat(float x, float drive) noexcept {
    const float d = 1.0f + drive * 4.0f;
    const float b = 0.08f * drive;
    return (std::tanh(d * x + b) - std::tanh(b)) / d;
}

float Compressor::msToCoeff(float ms, double sr) noexcept {
    const float tau = juce::jmax(1.0e-4f, ms * 1.0e-3f);
    return std::exp(-1.0f / (tau * (float) sr));
}
float Compressor::logLerp(float a, float b, float t) noexcept {
    return std::exp(juce::jmap(juce::jlimit(0.0f, 1.0f, t),
                               std::log(a), std::log(b)));
}

void Compressor::prepare(double sampleRate, int) noexcept {
    sr_ = sampleRate;
    rmsCoeff_ = msToCoeff(30.0f, sr_);
    scHpCoeff_ = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi
                                 * 120.0f / (float) sr_);
    reset();
}

void Compressor::reset() noexcept {
    envGain_ = 1.0f; rmsState_ = 0.0f; scHpState_ = 0.0f;
    lastGrDb_ = 0.0f; lastReduction_ = 0.0f;
}

void Compressor::setThreshold(float db) noexcept { thresholdDb_ = db; }

void Compressor::setBehavior(float pos) noexcept {
    beh_ = juce::jlimit(0.0f, 1.0f, pos / 10.0f);
    baseRatio_ = 1.5f + beh_ * beh_ * 18.5f;
}

void Compressor::setMakeup(float pos) noexcept {
    makeupGain_ = scales::makeupGain(pos);
    makeupDrive_ = juce::jmax(0.0f, (pos - 5.0f) / 5.0f);
}

void Compressor::setTiming(int atk, int rel) noexcept {
    static constexpr float atkMs[3] = { 0.5f, 5.0f, 30.0f };
    static constexpr float relMs[3] = { 60.0f, 250.0f, 900.0f };
    baseAtkMs_ = atkMs[juce::jlimit(0, 2, atk)];
    baseRelMs_ = relMs[juce::jlimit(0, 2, rel)];
}

void Compressor::setRmsMode(bool on) noexcept { rmsMode_ = on; }
void Compressor::setScFilter(bool on) noexcept { scFilterOn_ = on; }
void Compressor::setVcaCharacter(float amount01) noexcept {
    vcaChar_ = juce::jlimit(0.0f, 1.0f, amount01);
}

void Compressor::processStereo(float& l, float& r, float scL, float scR) noexcept {
    // 1) linked detector input
    float det = juce::jmax(std::abs(scL), std::abs(scR));
    if (scFilterOn_) {
        scHpState_ += scHpCoeff_ * (det - scHpState_); // one-pole LP of magnitude
        det = juce::jmax(0.0f, det - scHpState_);       // subtract => HPF
    }
    det += kDetectorFeedback * lastReduction_;

    // 2) level estimate (single one-pole RMS when enabled)
    float levelDb;
    if (rmsMode_) {
        rmsState_ = rmsCoeff_ * rmsState_ + (1.0f - rmsCoeff_) * (det * det);
        levelDb = juce::Decibels::gainToDecibels(std::sqrt(rmsState_ + 1.0e-12f));
    } else {
        levelDb = juce::Decibels::gainToDecibels(det + 1.0e-12f);
    }

    // 3) gain computer (program-dependent ratio)
    const float over = levelDb - thresholdDb_;
    float targetGrDb = 0.0f;
    if (over > 0.0f) {
        const float ratio = baseRatio_ * (1.0f + beh_ * over * 0.05f);
        targetGrDb = over * (1.0f - 1.0f / ratio);
    }
    const float targetGain = juce::Decibels::decibelsToGain(-targetGrDb);

    // 4) program-dependent timing
    float atkCoeff, relCoeff;
    if (rmsMode_) {
        atkCoeff = msToCoeff(10.0f, sr_);
        relCoeff = msToCoeff(300.0f, sr_);
    } else {
        const float frac = juce::jlimit(0.0f, 1.0f, (targetGrDb / 20.0f) * beh_);
        atkCoeff = msToCoeff(logLerp(baseAtkMs_, kFastAtkFloorMs, frac), sr_);
        relCoeff = msToCoeff(logLerp(baseRelMs_, kFastRelFloorMs, frac), sr_);
    }
    const float coeff = (targetGain < envGain_) ? atkCoeff : relCoeff;
    envGain_ = coeff * envGain_ + (1.0f - coeff) * targetGain;

    // 5) apply VCA gain
    const float g = envGain_;
    lastReduction_ = 1.0f - g;
    lastGrDb_ = -juce::Decibels::gainToDecibels(g);

    // 5b) VCA harmonic distortion that tracks gain reduction (thicker the harder it
    // clamps). Zero when character is off or there is no reduction. The small per-channel
    // offset de-correlates L/R; detection above stays linked, so the image is stable.
    // Applied before the gain multiply so the shaper operates at full signal level.
    const float grDrive = vcaChar_ * lastReduction_;
    if (grDrive > 0.0f) {
        constexpr float kVcaChOffset = 0.05f;
        l = vcaSat(l, grDrive * (1.0f + kVcaChOffset));
        r = vcaSat(r, grDrive * (1.0f - kVcaChOffset));
    }

    l *= g; r *= g;

    // 6) makeup (+ soft nonlinearity past unity)
    l *= makeupGain_; r *= makeupGain_;
    if (makeupDrive_ > 0.0f) { l = softSat(l, makeupDrive_); r = softSat(r, makeupDrive_); }
}

} // namespace holdover
