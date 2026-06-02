#pragma once
#include "Scales.h"

namespace holdover {

// Sums the three parallel feeds, each with a continuous level and a mute.
class FeedMatrix {
public:
    void setDryEq(float levelPos, bool mute) noexcept { dryEq_ = gain(levelPos, mute); }
    void setComp(float levelPos, bool mute)  noexcept { comp_  = gain(levelPos, mute); }
    void setSat(float levelPos, bool mute)   noexcept { sat_   = gain(levelPos, mute); }

    float mix(float dryEqFeed, float compFeed, float satFeed) const noexcept {
        return dryEqFeed * dryEq_ + compFeed * comp_ + satFeed * sat_;
    }
private:
    static float gain(float levelPos, bool mute) noexcept {
        return mute ? 0.0f : scales::feedGain(levelPos);
    }
    float dryEq_ = 0.0f, comp_ = 0.0f, sat_ = 1.0f;
};

} // namespace holdover
