#include <catch2/catch_test_macros.hpp>
#include "presets/FactoryPresets.h"
#include "Parameters.h"

TEST_CASE("factory bank has the expected presets", "[presets]") {
    REQUIRE(holdover::presets::all().size() == 11);
}

TEST_CASE("every preset references only valid parameter IDs and finite values", "[presets]") {
    const auto& ids = holdover::params::allIDs();
    juce::StringArray names;
    for (const auto& preset : holdover::presets::all()) {
        names.add(preset.name);
        REQUIRE(preset.name.isNotEmpty());
        for (const auto& [id, value] : preset.values) {
            REQUIRE(ids.contains(id));
            REQUIRE(std::isfinite(value));
        }
    }
    names.removeDuplicates(false);
    REQUIRE(names.size() == (int) holdover::presets::all().size());
}
