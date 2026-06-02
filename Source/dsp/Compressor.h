#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace holdover {

// Feed-forward "clean VCA" compressor. No ratio control: THRESHOLD + BEHAVIOR
// define the compression. BEHAVIOR nonlinearly magnifies gain reduction and skews
// attack/release timing keyed to the instantaneous GR amount (program-dependent).
class Compressor {
public:
    void prepare(double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    void setThreshold(float thresholdDb) noexcept;
    void setBehavior(float pos0to10) noexcept;
    void setMakeup(float pos0to10) noexcept;
    void setTiming(int attackIdx, int releaseIdx) noexcept; // 0=F,1=M,2=S
    void setRmsMode(bool on) noexcept;
    void setScFilter(bool on) noexcept;

    void processStereo(float& l, float& r, float scL, float scR) noexcept;

    float getGainReductionDb() const noexcept { return lastGrDb_; } // positive dB

private:
    static float msToCoeff(float ms, double sr) noexcept;
    static float logLerp(float a, float b, float t) noexcept;

    static constexpr float kDetectorFeedback = 0.0f;
    static constexpr float kFastAtkFloorMs = 0.1f;
    static constexpr float kFastRelFloorMs = 20.0f;

    double sr_ = 48000.0;
    float thresholdDb_ = 0.0f;
    float beh_ = 0.0f;
    float baseRatio_ = 1.5f;
    float makeupGain_ = 1.0f;
    float makeupDrive_ = 0.0f;
    float baseAtkMs_ = 5.0f, baseRelMs_ = 250.0f;
    bool rmsMode_ = false, scFilterOn_ = false;

    float envGain_ = 1.0f;
    float rmsState_ = 0.0f, rmsCoeff_ = 0.0f;
    float scHpState_ = 0.0f, scHpCoeff_ = 0.0f;
    float lastGrDb_ = 0.0f, lastReduction_ = 0.0f;
};

} // namespace holdover
