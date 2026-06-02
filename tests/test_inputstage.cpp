#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/InputStage.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("input stage is unity at position 5", "[input]") {
    holdover::InputStage in;
    in.setGains(5.0f, 5.0f, 5.0f);
    float l = 0.7f, r = -0.3f;
    in.processStereo(l, r);
    REQUIRE_THAT(l, WithinAbs(0.7f, 1e-4f));
    REQUIRE_THAT(r, WithinAbs(-0.3f, 1e-4f));
}

TEST_CASE("per-channel trim is independent", "[input]") {
    holdover::InputStage in;
    in.setGains(10.0f, 0.0f, 5.0f);
    float l = 0.1f, r = 0.1f;
    in.processStereo(l, r);
    REQUIRE(l > r);
}
