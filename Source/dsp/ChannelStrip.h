#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "InputStage.h"
#include "FilterBlock.h"
#include "EqBlock.h"
#include "Compressor.h"
#include "DriveBlock.h"
#include "FeedMatrix.h"
#include "CeilingLimiter.h"
#include "OutputStage.h"
#include "MeterState.h"

namespace holdover {

// Integrates all modules, applies the routing, and oversamples the whole matrix
// region once (4x, minimum-phase IIR). All feeds share one latency.
class ChannelStrip {
public:
    struct Targets {
        float gainLpos, gainRpos, inputPos;
        float hpfFreq, hpfPeak, lpfFreq, lpfPeak;
        float presenceDb, presenceFreq, bassDb, bassFreq, trebleDb, trebleFreq;
        float thresholdDb, behaviorPos, makeupPos;
        int attackIdx, releaseIdx;
        float drivePos; int masMode;
        float characterPos;
        float dryEqFeedPos, compFeedPos, satFeedPos, outputPos;
        int scSource;     // 0 pre, 1 ext, 2 post
        int dryEqSource;  // 0 pre, 1 post
        bool filtEngage, eqEngage, compEngage, curve, rmsMode, scFilter,
             satEngage, hexEngage, dryEqMute, compMute, satMute, ceiling;
    };

    void prepare(double sampleRate, int maxBlockSize, int numChannels) noexcept;
    void reset() noexcept;
    void setTargets(const Targets&) noexcept;

    void processBlock(juce::AudioBuffer<float>& main,
                      const juce::AudioBuffer<float>* sidechain) noexcept;

    int getLatencySamples() const noexcept { return latencySamples_; }
    const MeterState& meters() const noexcept { return meters_; }

private:
    void applyTargets() noexcept;
    void processOversampled(juce::dsp::AudioBlock<float>& up,
                            juce::dsp::AudioBlock<float>& upSc) noexcept;

    static constexpr int kFactor = 2; // 2 stages => 4x

    double sr_ = 48000.0;
    int numCh_ = 2, latencySamples_ = 0;
    Targets t_ {};
    bool haveTargets_ = false;

    InputStage input_;
    std::array<FilterBlock, 2> filter_;
    std::array<EqBlock, 2> eq_;
    Compressor comp_;
    std::array<DriveBlock, 2> drive_;
    FeedMatrix matrix_;
    std::array<CeilingLimiter, 2> ceil_;
    OutputStage output_;
    MeterState meters_;

    std::unique_ptr<juce::dsp::Oversampling<float>> os_, scOs_;
    juce::AudioBuffer<float> scBaseBuffer_;  // preallocated base-rate SC scratch
    float satActivity_ = 0.0f;
};

} // namespace holdover
