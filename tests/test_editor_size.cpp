#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace holdover;

// Regression guard for the editor's initial-size handling. The bug: the constructor
// configured the resize constrainer (setResizeLimits → setBoundsConstrained on the
// still-0x0 editor → clamp to minimum → resized()) BEFORE reading processor.uiWidth/
// uiHeight in setSize(), so the clamp overwrote the persisted size and the window
// always opened at the minimum. This manifested as "opens too small" and as size
// being lost after the editor was closed and reopened.

TEST_CASE("a fresh editor opens at the full base size", "[editor]") {
    juce::ScopedJuceInitialiser_GUI guiInit;
    HoldoverProcessor proc; // defaults: uiWidth = kBaseWidth, uiHeight = kBaseHeight
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    REQUIRE(ed->getWidth()  == HoldoverEditor::kBaseWidth);
    REQUIRE(ed->getHeight() == HoldoverEditor::kBaseHeight);
}

TEST_CASE("a reopened editor restores the persisted size", "[editor]") {
    juce::ScopedJuceInitialiser_GUI guiInit;
    HoldoverProcessor proc;
    // Simulate a prior user resize to an in-limits, on-aspect (3:2) size.
    proc.uiWidth  = 1290; // 1290 / 860 = 1.5, within [714, 1632]
    proc.uiHeight = 860;
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    REQUIRE(ed->getWidth()  == 1290);
    REQUIRE(ed->getHeight() == 860);
    // And the processor still reflects that size after construction.
    REQUIRE(proc.uiWidth  == 1290);
    REQUIRE(proc.uiHeight == 860);
}
