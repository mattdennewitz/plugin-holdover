#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/Scales.h"

using Catch::Matchers::WithinAbs;
using namespace holdover::scales;

TEST_CASE("fader law is unity at position 5", "[scales]") {
    REQUIRE_THAT(faderGain(5.0f), WithinAbs(1.0f, 1e-4f));     // 0 dB
    REQUIRE(faderGain(0.0f) < 1.0e-3f);                        // ~ -inf (silence)
    REQUIRE(faderGain(10.0f) > 1.0f);                          // boost above unity
    REQUIRE(faderGain(2.0f) < faderGain(3.0f));                // monotonic
}

TEST_CASE("trim law spans +/-24 dB, unity at 5", "[scales]") {
    REQUIRE_THAT(trimGain(5.0f), WithinAbs(1.0f, 1e-4f));
    REQUIRE_THAT(trimGain(10.0f), WithinAbs(juce::Decibels::decibelsToGain(24.0f), 1e-3f));
    REQUIRE_THAT(trimGain(0.0f),  WithinAbs(juce::Decibels::decibelsToGain(-24.0f), 1e-3f));
}

TEST_CASE("feed law is silent at 0 and unity at 10", "[scales]") {
    REQUIRE(feedGain(0.0f) < 1.0e-3f);
    REQUIRE_THAT(feedGain(10.0f), WithinAbs(1.0f, 1e-4f));
}

TEST_CASE("drive pre-gain is unity at 5 and rises above", "[scales]") {
    REQUIRE_THAT(drivePreGain(5.0f), WithinAbs(1.0f, 1e-4f));
    REQUIRE(drivePreGain(10.0f) > drivePreGain(5.0f));
}
