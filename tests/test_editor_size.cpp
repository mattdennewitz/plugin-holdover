#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
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

// setSize() does not run the constrainer, so a size loaded from an older session
// (pre-rebuild defaults, or the old 2x resize ceiling) could open out of range or
// off the now-locked aspect ratio. The constructor clamps width into the limits and
// derives height from the aspect ratio, so the window never opens oversized or with
// a gap below the content.

TEST_CASE("an oversized persisted size is clamped into the resize limits", "[editor]") {
    juce::ScopedJuceInitialiser_GUI guiInit;
    HoldoverProcessor proc;
    proc.uiWidth  = 3000; // far beyond the 1.6x ceiling
    proc.uiHeight = 2000;
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    const int maxW = (int) std::round(HoldoverEditor::kBaseWidth * 1.6);  // 1632
    const int maxH = (int) std::round(maxW / ((double) HoldoverEditor::kBaseWidth
                                              / (double) HoldoverEditor::kBaseHeight));
    REQUIRE(ed->getWidth()  == maxW);
    REQUIRE(ed->getHeight() == maxH);
}

TEST_CASE("an off-aspect persisted size opens on the locked aspect ratio", "[editor]") {
    juce::ScopedJuceInitialiser_GUI guiInit;
    HoldoverProcessor proc;
    // Old default: 900x700 (aspect ~1.286), in width range but off the 1.5 aspect.
    proc.uiWidth  = 900;
    proc.uiHeight = 700;
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    REQUIRE(ed->getWidth()  == 900);  // width is in range, so preserved
    REQUIRE(ed->getHeight() == 600);  // 900 / 1.5 — height corrected to the aspect
}
