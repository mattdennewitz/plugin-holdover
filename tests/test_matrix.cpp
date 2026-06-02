#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/FeedMatrix.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("muted feed contributes nothing", "[matrix]") {
    holdover::FeedMatrix m;
    m.setDryEq(10.0f, true);
    m.setComp(0.0f, true);
    m.setSat(10.0f, false);
    REQUIRE_THAT(m.mix(0.5f, 0.9f, 0.2f), WithinAbs(0.2f, 1e-4f));
}

TEST_CASE("active feeds sum at their levels", "[matrix]") {
    holdover::FeedMatrix m;
    m.setDryEq(10.0f, false);
    m.setComp(10.0f, false);
    m.setSat(0.0f, true);
    REQUIRE_THAT(m.mix(0.3f, 0.4f, 1.0f), WithinAbs(0.7f, 1e-3f));
}
