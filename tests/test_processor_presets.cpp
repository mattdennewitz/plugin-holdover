#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "PluginProcessor.h"
#include "presets/FactoryPresets.h"

using Catch::Approx;

TEST_CASE("host program interface delegates to the preset manager", "[processor]") {
    holdover::HoldoverProcessor proc;

    REQUIRE(proc.getNumPrograms() == (int) holdover::presets::all().size());

    proc.setCurrentProgram(3); // "Bass Saturator" -> drive=8
    REQUIRE(proc.getCurrentProgram() == 3);
    REQUIRE(proc.getProgramName(3) == "Bass Saturator");
    REQUIRE(proc.apvts.getRawParameterValue("drive")->load() == Approx(8.0f));
    REQUIRE(proc.getPresetManager().getCurrentIndex() == 3);
    REQUIRE(proc.currentPresetName == "Bass Saturator");
}

TEST_CASE("currentPresetName defaults to the first factory preset", "[processor]") {
    holdover::HoldoverProcessor proc;
    REQUIRE(proc.currentPresetName == "Init / Flat");
}

TEST_CASE("currentPresetName round-trips through plugin state", "[processor]") {
    holdover::HoldoverProcessor a;
    a.currentPresetName = "Vocal Glue";

    juce::MemoryBlock mb;
    a.getStateInformation(mb);

    holdover::HoldoverProcessor b;
    b.setStateInformation(mb.getData(), (int) mb.getSize());
    REQUIRE(b.currentPresetName == "Vocal Glue");
}
