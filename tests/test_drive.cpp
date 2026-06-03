#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/DriveBlock.h"
#include "../tests/TestSignals.h"
#include <numeric>

namespace { constexpr double kSr = 192000.0; }

TEST_CASE("CURVE is inaudible at zero drive with stages off", "[drive]") {
    holdover::DriveBlock d; d.prepare(kSr); d.reset();
    d.setDrive(5.0f);
    d.setMas(0); d.setSat(false); d.setHex(false);
    d.setCurve(true);
    const double w = 2.0 * juce::MathConstants<double>::pi * 1000.0 / kSr;
    float maxErr = 0.0f;
    for (int n = 0; n < 8192; ++n) {
        const float x = 0.5f * (float) std::sin(w * n);
        maxErr = juce::jmax(maxErr, std::abs(d.processSample(x) - x));
    }
    REQUIRE(maxErr < 1.0e-3f);
}

TEST_CASE("MAS 2nd emphasizes even harmonics", "[drive]") {
    holdover::DriveBlock d; d.prepare(kSr); d.reset();
    d.setDrive(8.0f); d.setMas(1); d.setSat(false); d.setHex(false); d.setCurve(false);
    auto h = test::harmonicMagnitudes([&](float x){ return d.processSample(x); },
                                      kSr, 1000.0f, 5);
    REQUIRE(h[1] > h[2]);
    REQUIRE(h[1] > h[0] * 0.02f);
}

TEST_CASE("MAS 3rd emphasizes odd harmonics", "[drive]") {
    holdover::DriveBlock d; d.prepare(kSr); d.reset();
    d.setDrive(8.0f); d.setMas(2); d.setSat(false); d.setHex(false); d.setCurve(false);
    auto h = test::harmonicMagnitudes([&](float x){ return d.processSample(x); },
                                      kSr, 1000.0f, 5);
    REQUIRE(h[2] > h[1]);
}

TEST_CASE("HEX generates more high-order content than SAT", "[drive]") {
    auto highOrderEnergy = [](bool hex, bool sat) {
        holdover::DriveBlock d; d.prepare(kSr); d.reset();
        d.setDrive(8.0f); d.setMas(0); d.setSat(sat); d.setHex(hex); d.setCurve(false);
        auto h = test::harmonicMagnitudes([&](float x){ return d.processSample(x); },
                                          kSr, 1000.0f, 9);
        return std::accumulate(h.begin() + 4, h.end(), 0.0f);
    };
    REQUIRE(highOrderEnergy(true, false) > highOrderEnergy(false, true));
}

TEST_CASE("Class-A bias adds 2nd-harmonic growl scaled by character", "[drive]") {
    auto secondHarmonicRatio = [](float character) {
        holdover::DriveBlock d; d.prepare(kSr); d.reset();
        d.setDrive(8.0f); d.setMas(0); d.setSat(true); d.setHex(false); d.setCurve(false);
        d.setCharacter(character, +1.0f);
        auto h = test::harmonicMagnitudes([&](float x){ return d.processSample(x); },
                                          kSr, 1000.0f, 5);
        return h[1] / h[0];   // 2nd harmonic relative to fundamental
    };
    const float clean   = secondHarmonicRatio(0.0f);  // SAT alone (odd tanh) => ~no 2nd
    const float colored = secondHarmonicRatio(1.0f);
    REQUIRE(clean < 0.01f);
    REQUIRE(colored > 0.02f);
    REQUIRE(colored > clean * 3.0f);
}

TEST_CASE("Class-A bias de-correlates L/R yet survives a mono sum", "[drive]") {
    // L/R use a shared in-phase bias base plus a small opposite-sign trim, so the channels
    // differ (stereo width) but their even-harmonic content does NOT cancel when summed to
    // mono. A pure +/- antiphase bias would pass the de-correlation check below but fail the
    // mono-compatibility check.
    auto makeChannel = [](float chSign) {
        auto d = std::make_unique<holdover::DriveBlock>();
        d->prepare(kSr); d->reset();
        d->setDrive(8.0f); d->setMas(0); d->setSat(true); d->setHex(false); d->setCurve(false);
        d->setCharacter(1.0f, chSign);
        return d;
    };

    // (a) de-correlation: per-sample L and R outputs are not identical.
    auto dl = makeChannel(+1.0f), dr = makeChannel(-1.0f);
    const double w = 2.0 * juce::MathConstants<double>::pi * 1000.0 / kSr;
    float maxDiff = 0.0f;
    for (int n = 0; n < 2048; ++n) {
        const float x = 0.5f * (float) std::sin(w * n);
        maxDiff = juce::jmax(maxDiff, std::abs(dl->processSample(x) - dr->processSample(x)));
    }
    REQUIRE(maxDiff > 1.0e-4f);

    // (b) mono compatibility: the 2nd harmonic of the (L+R)/2 mono sum is still present.
    auto ml = makeChannel(+1.0f), mr = makeChannel(-1.0f);
    auto h = test::harmonicMagnitudes(
        [&](float x){ return 0.5f * (ml->processSample(x) + mr->processSample(x)); },
        kSr, 1000.0f, 5);
    REQUIRE(h[1] > h[0] * 0.02f);   // even-harmonic warmth survives the mono fold
}

TEST_CASE("Class-A bias asymmetry scales with drive", "[drive]") {
    // Same Character, lower Drive => less signal (and bias) pushed into the shaper => less
    // 2nd-harmonic growl. This is the Class-A trait: the asymmetry blooms as the stage is
    // pushed, rather than sitting at a fixed level regardless of drive.
    auto secondRatioAtDrive = [](float drivePos) {
        holdover::DriveBlock d; d.prepare(kSr); d.reset();
        d.setDrive(drivePos); d.setMas(0); d.setSat(true); d.setHex(false); d.setCurve(false);
        d.setCharacter(1.0f, +1.0f);
        auto h = test::harmonicMagnitudes([&](float x){ return d.processSample(x); },
                                          kSr, 1000.0f, 5);
        return h[1] / h[0];
    };
    REQUIRE(secondRatioAtDrive(8.0f) > secondRatioAtDrive(3.0f));
}
