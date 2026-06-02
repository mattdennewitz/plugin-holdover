#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/Compressor.h"
#include <cmath>

namespace {
constexpr double kSr = 48000.0;

float steadyGainDb(holdover::Compressor& c, float ampDb, int n = 48000) {
    c.reset();
    const float amp = juce::Decibels::decibelsToGain(ampDb);
    const double w = 2.0 * juce::MathConstants<double>::pi * 200.0 / kSr;
    double inAcc = 0, outAcc = 0; int counted = 0;
    for (int i = 0; i < n; ++i) {
        float s = amp * (float) std::sin(w * i);
        float l = s, r = s;
        c.processStereo(l, r, s, s);
        if (i >= n / 2) { inAcc += (double) s * s; outAcc += (double) l * l; ++counted; }
    }
    return juce::Decibels::gainToDecibels((float) std::sqrt(outAcc / counted)
                                        / (float) std::sqrt(inAcc / counted));
}
}

TEST_CASE("no gain reduction below threshold", "[comp]") {
    holdover::Compressor c; c.prepare(kSr, 512);
    c.setThreshold(-10.0f); c.setBehavior(5.0f); c.setMakeup(5.0f);
    c.setTiming(1, 1); c.setRmsMode(false);
    REQUIRE_THAT(steadyGainDb(c, -20.0f), Catch::Matchers::WithinAbs(0.0f, 0.5f));
}

TEST_CASE("gain reduction above threshold; behavior increases it", "[comp]") {
    holdover::Compressor c; c.prepare(kSr, 512);
    c.setThreshold(-20.0f); c.setMakeup(5.0f); c.setTiming(0, 0); c.setRmsMode(false);
    c.setBehavior(2.0f);
    const float gentle = steadyGainDb(c, -6.0f);
    c.setBehavior(9.0f);
    const float aggressive = steadyGainDb(c, -6.0f);
    REQUIRE(gentle < -0.5f);
    REQUIRE(aggressive < gentle);
}

TEST_CASE("fast attack reaches target sooner than slow attack", "[comp]") {
    auto samplesToHalfGr = [](int atkIdx) {
        holdover::Compressor c; c.prepare(kSr, 512);
        c.setThreshold(-20.0f); c.setBehavior(0.0f); c.setMakeup(5.0f);
        c.setTiming(atkIdx, 1); c.setRmsMode(false); c.reset();
        const float amp = juce::Decibels::decibelsToGain(-3.0f);
        const double w = 2.0 * juce::MathConstants<double>::pi * 1000.0 / kSr;
        for (int i = 0; i < 48000; ++i) {
            float s = amp * (float) std::sin(w * i), l = s, r = s;
            c.processStereo(l, r, s, s);
            if (c.getGainReductionDb() > 1.0f) return i;
        }
        return 48000;
    };
    REQUIRE(samplesToHalfGr(0) < samplesToHalfGr(2));
}

TEST_CASE("SC filter reduces reduction for low-frequency content", "[comp]") {
    auto grAt = [](float freq, bool scFilter) {
        holdover::Compressor c; c.prepare(kSr, 512);
        c.setThreshold(-25.0f); c.setBehavior(5.0f); c.setMakeup(5.0f);
        c.setTiming(0, 1); c.setRmsMode(false); c.setScFilter(scFilter); c.reset();
        const float amp = juce::Decibels::decibelsToGain(-6.0f);
        const double w = 2.0 * juce::MathConstants<double>::pi * freq / kSr;
        float gr = 0;
        for (int i = 0; i < 48000; ++i) {
            float s = amp * (float) std::sin(w * i), l = s, r = s;
            c.processStereo(l, r, s, s);
            if (i > 24000) gr = juce::jmax(gr, c.getGainReductionDb());
        }
        return gr;
    };
    REQUIRE(grAt(50.0f, true) < grAt(50.0f, false));
}

TEST_CASE("RMS detection reads lower than peak for steady tone", "[comp]") {
    // Peak level of a -6 dBFS sine is -6 dB; its RMS is ~-9 dB. With threshold at
    // -7.5 dB, peak mode compresses but RMS mode (reading lower) does not.
    auto grWithMode = [](bool rms) {
        holdover::Compressor c; c.prepare(48000.0, 512);
        c.setThreshold(-7.5f); c.setBehavior(6.0f); c.setMakeup(5.0f);
        c.setTiming(0, 0); c.setRmsMode(rms); c.reset();
        const float amp = juce::Decibels::decibelsToGain(-6.0f);
        const double w = 2.0 * juce::MathConstants<double>::pi * 200.0 / 48000.0;
        float gr = 0.0f;
        for (int i = 0; i < 48000; ++i) {
            float s = amp * (float) std::sin(w * i), l = s, r = s;
            c.processStereo(l, r, s, s);
            if (i > 24000) gr = juce::jmax(gr, c.getGainReductionDb());
        }
        return gr;
    };
    REQUIRE(grWithMode(false) > grWithMode(true) + 0.5f); // peak compresses more than RMS
}
