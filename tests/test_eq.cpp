#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/EqBlock.h"
#include "../tests/TestSignals.h"

namespace { constexpr double kSr = 48000.0; }
using Catch::Matchers::WithinAbs;

TEST_CASE("low shelf boosts lows by the requested gain", "[eq]") {
    holdover::EqBlock eq; eq.prepare(kSr); eq.reset();
    eq.setBass(6.0f, 150.0f);
    eq.setPresence(0.0f, 2000.0f);
    eq.setTreble(0.0f, 10000.0f);
    const float lowDb = test::magnitudeDb([&](float x){ return eq.processSample(x); }, kSr, 40.0f);
    REQUIRE_THAT(lowDb, WithinAbs(6.0f, 1.5f));
}

TEST_CASE("presence Q tightens as boost magnitude increases", "[eq]") {
    auto bandwidthDb3 = [](float gainDb) {
        holdover::EqBlock eq; eq.prepare(kSr); eq.reset();
        eq.setBass(0.0f, 150.0f); eq.setTreble(0.0f, 10000.0f);
        eq.setPresence(gainDb, 2000.0f);
        const float peak = test::magnitudeDb([&](float x){ return eq.processSample(x); }, kSr, 2000.0f);
        float f = 2000.0f;
        for (; f < 20000.0f; f *= 1.01f) {
            eq.reset();
            const float m = test::magnitudeDb([&](float x){ return eq.processSample(x); }, kSr, f);
            if (m <= peak - 3.0f) break;
        }
        return f / 2000.0f;
    };
    REQUIRE(bandwidthDb3(12.0f) < bandwidthDb3(3.0f));
}

TEST_CASE("flat EQ is unity", "[eq]") {
    holdover::EqBlock eq; eq.prepare(kSr); eq.reset();
    eq.setBass(0.0f, 150.0f); eq.setPresence(0.0f, 2000.0f); eq.setTreble(0.0f, 10000.0f);
    const float m = test::magnitudeDb([&](float x){ return eq.processSample(x); }, kSr, 1000.0f);
    REQUIRE_THAT(m, WithinAbs(0.0f, 0.2f));
}
