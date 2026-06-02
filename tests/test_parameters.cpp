#include <catch2/catch_test_macros.hpp>
#include "Parameters.h"

TEST_CASE("parameter layout contains every spec parameter", "[params]") {
    auto layout = holdover::params::createLayout();
    const juce::StringArray expected {
        "gainL","gainR","input","hpfFreq","hpfPeak","lpfFreq","lpfPeak",
        "presenceDb","presenceFreq","bassDb","trebleDb","threshold","behavior",
        "makeup","drive","dryEqFeedLevel","compFeedLevel","satFeedLevel","output",
        "bassFreq","trebleFreq","attack","release","masMode","scSource","dryEqSource",
        "filtEngage","eqEngage","compEngage","curve","rmsMode","scFilter",
        "satEngage","hexEngage","dryEqMute","compMute","satMute","ceiling" };
    REQUIRE(holdover::params::allIDs().size() == expected.size());
    for (auto& id : expected)
        REQUIRE(holdover::params::allIDs().contains(id));
}
