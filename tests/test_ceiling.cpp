#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/CeilingLimiter.h"
#include <cmath>

namespace { constexpr double kSr = 48000.0; }

TEST_CASE("disengaged ceiling is passthrough", "[ceiling]") {
    holdover::CeilingLimiter c; c.prepare(kSr); c.reset(); c.setEngaged(false);
    REQUIRE_THAT(c.processSample(3.0f), Catch::Matchers::WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("engaged ceiling bounds loud signals", "[ceiling]") {
    holdover::CeilingLimiter c; c.prepare(kSr); c.reset(); c.setEngaged(true);
    float peak = 0.0f;
    const double w = 2.0 * juce::MathConstants<double>::pi * 200.0 / kSr;
    for (int n = 0; n < 4096; ++n)
        peak = juce::jmax(peak, std::abs(c.processSample(4.0f * (float) std::sin(w * n))));
    REQUIRE(peak <= 1.05f);
}
