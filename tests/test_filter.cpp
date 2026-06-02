#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/FilterBlock.h"
#include "../tests/TestSignals.h"

namespace {
constexpr double kSr = 48000.0;
float runOne(holdover::FilterBlock& f, float x) { return f.processSample(x); }
}

TEST_CASE("HPF attenuates lows and passes highs", "[filter]") {
    holdover::FilterBlock f; f.prepare(kSr); f.reset();
    f.setHpf(500.0f, 0.0f);
    f.setLpf(22000.0f, 0.0f);
    const float low  = test::magnitudeDb([&](float x){ return runOne(f, x); }, kSr, 50.0f);
    f.reset();
    const float high = test::magnitudeDb([&](float x){ return runOne(f, x); }, kSr, 5000.0f);
    REQUIRE(low < -12.0f);
    REQUIRE(high > -1.0f);
}

TEST_CASE("resonance creates a peak near LPF cutoff", "[filter]") {
    holdover::FilterBlock f; f.prepare(kSr); f.reset();
    f.setHpf(20.0f, 0.0f);
    f.setLpf(1000.0f, 7.0f);
    const float atCutoff = test::magnitudeDb([&](float x){ return runOne(f, x); }, kSr, 1000.0f);
    REQUIRE(atCutoff > 3.0f);
}

TEST_CASE("self-oscillation is bounded (no runaway)", "[filter]") {
    holdover::FilterBlock f; f.prepare(kSr); f.reset();
    f.setHpf(20.0f, 0.0f);
    f.setLpf(1000.0f, 10.0f);
    float peak = 0.0f, y = 0.0f;
    y = f.processSample(1.0f);
    for (int n = 0; n < 96000; ++n) {
        y = f.processSample(0.0f);
        peak = juce::jmax(peak, std::abs(y));
        REQUIRE(std::isfinite(y));
    }
    REQUIRE(peak > 0.05f);
    REQUIRE(peak < 4.0f);
}
