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
