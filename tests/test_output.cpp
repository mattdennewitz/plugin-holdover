#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/OutputStage.h"
#include "dsp/MeterState.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("output stage is unity at position 5", "[output]") {
    holdover::OutputStage o; o.setLevel(5.0f);
    float l = 0.42f, r = -0.5f; o.processStereo(l, r);
    REQUIRE_THAT(l, WithinAbs(0.42f, 1e-4f));
    REQUIRE_THAT(r, WithinAbs(-0.5f, 1e-4f));
}

TEST_CASE("meter state stores and loads atomically", "[meter]") {
    holdover::MeterState m;
    m.setSaturation(0.7f); m.setGainReductionDb(3.5f);
    REQUIRE_THAT(m.saturation(), WithinAbs(0.7f, 1e-6f));
    REQUIRE_THAT(m.gainReductionDb(), WithinAbs(3.5f, 1e-6f));
}
