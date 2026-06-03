# Character-Isolating Presets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add two factory presets ("Console Warmth", "Bus Glue Grit") that each isolate one of the Character macro's two mechanisms so its effect is obvious and the patch is musically useful.

**Architecture:** Pure data additions to the factory preset bank — two `Preset` entries appended to `holdover::presets::all()`, plus the count-assertion bump in the preset test. No DSP, UI, or parameter changes. The existing `[presets]` validity test (valid IDs, finite values, unique non-empty names) covers the new entries automatically.

**Tech Stack:** C++20, JUCE 8.0.4, CMake (Unix Makefiles), Catch2 v3.

---

## Context the implementer needs

- The factory bank is a single `static const std::vector<Preset>` returned by `holdover::presets::all()` in `Source/presets/FactoryPresets.cpp`. Each `Preset` is `{ name, { {paramID, realValue}, ... } }`. `apply()` converts each real value to normalized and notifies the host; params not listed keep their defaults.
- Signal chain: `in → filter → EQ → comp → drive`. Output matrix taps three feeds: **dryEq** (clean), **comp feed** (= comp output, carries only VCA THD), **sat feed** (= drive output, carries VCA THD + drive bias). Muting/leveling these selects what you hear.
- Character (`character`, 0–10) drives two things: a 2nd-harmonic bias into the Drive shaper (only audible when `masMode != 0` or SAT/HEX is on; cleanest with `masMode 1`) and VCA THD in the compressor that scales with gain reduction (only blooms when the comp clamps).
- **Param value types** (from `Source/Parameters.cpp`): floats use real values — `drive`, `character`, `threshold`, `behavior`, `makeup`, `output`, `*FeedLevel`. Choice params use integer indices — `masMode` (0=off,1=2nd,2=3rd), `attack`, `release`. Bool toggles are `0`/`1` — `compEngage`, `curve`, `satEngage`, `hexEngage`, `eqEngage`, `filtEngage`, `satMute`, `compMute`, `dryEqMute`.
- **Pre-existing uncommitted work:** `Source/presets/FactoryPresets.cpp` and `tests/test_presets.cpp` already have uncommitted edits on this branch (a "Character Showcase" preset, Character dialed into "Vocal Glue"/"Bass Saturator", and the count already bumped 8 → 9). `Source/dsp/DriveBlock.{cpp,h}` also have uncommitted DSP edits. This task adds to the two preset/test files **on top of** those edits and does NOT touch DriveBlock.

**Build/test commands** (run from repo root `/Users/matt/src/plugin-holdover`):
- Build headless tests: `cmake --build build --target HoldoverTests -j`
- Run the preset tests: `./build/tests/HoldoverTests "[presets]"`

---

## Task 1: Add the two Character-isolating presets

**Files:**
- Modify: `Source/presets/FactoryPresets.cpp` (append two `Preset` entries)
- Modify: `tests/test_presets.cpp:6` (count assertion 9 → 11)

- [ ] **Step 1: Update the count assertion (failing test first)**

In `tests/test_presets.cpp`, the first test currently reads:

```cpp
TEST_CASE("factory bank has the expected presets", "[presets]") {
    REQUIRE(holdover::presets::all().size() == 9);
}
```

Change the `9` to `11`:

```cpp
TEST_CASE("factory bank has the expected presets", "[presets]") {
    REQUIRE(holdover::presets::all().size() == 11);
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target HoldoverTests -j && ./build/tests/HoldoverTests "[presets]"`
Expected: FAIL — the count test reports the bank size is 9, not 11. (`REQUIRE(...size() == 11)` fails.)

- [ ] **Step 3: Append the two presets**

In `Source/presets/FactoryPresets.cpp`, the bank currently ends with the "Character Showcase" entry followed by the closing brace of the initializer list:

```cpp
        { "Character Showcase", {
            {"compEngage",1},{"threshold",-26},{"behavior",9},{"attack",0},{"release",0},
            {"makeup",7},{"drive",6},{"masMode",1},{"curve",1},{"character",10},
            {"satEngage",0},{"hexEngage",0},{"eqEngage",0},{"filtEngage",0},
            {"satFeedLevel",10},{"satMute",0},{"dryEqMute",1},{"compMute",1},{"output",5} } },
    };
    return bank;
}
```

Insert the two new presets between the "Character Showcase" entry's closing `} },` and the `};` that closes the list:

```cpp
        { "Character Showcase", {
            {"compEngage",1},{"threshold",-26},{"behavior",9},{"attack",0},{"release",0},
            {"makeup",7},{"drive",6},{"masMode",1},{"curve",1},{"character",10},
            {"satEngage",0},{"hexEngage",0},{"eqEngage",0},{"filtEngage",0},
            {"satFeedLevel",10},{"satMute",0},{"dryEqMute",1},{"compMute",1},{"output",5} } },

        // Isolates Character's Drive-bias mechanism: the compressor is OFF and SAT/HEX
        // are off, so the only thing coloring the audible (sat-feed = drive-output) path
        // is the Character-biased MAS-2nd shaper. Sweep Character 0 -> 8 to hear pure
        // even-harmonic "Class-A" warmth bloom. Useful on a DI, synth, or mix bus.
        { "Console Warmth", {
            {"compEngage",0},{"drive",5},{"masMode",1},{"curve",1},{"character",8},
            {"satEngage",0},{"hexEngage",0},{"eqEngage",0},{"filtEngage",0},
            {"satFeedLevel",10},{"satMute",0},{"compMute",1},{"dryEqMute",1},{"output",5} } },

        // Isolates Character's Comp-VCA-THD mechanism: masMode is OFF (Drive is identity)
        // and SAT/HEX are off, so the compressor is the only harmonic source. The comp
        // feed carries grDrive = Character x gainReduction. Sweep Character 0 -> 8 to hear
        // clean glue turn into gritty "British bus" bite that tracks how hard it clamps.
        { "Bus Glue Grit", {
            {"compEngage",1},{"threshold",-22},{"behavior",5},{"attack",1},{"release",2},
            {"makeup",7},{"masMode",0},{"satEngage",0},{"hexEngage",0},{"character",8},
            {"eqEngage",0},{"filtEngage",0},
            {"compFeedLevel",10},{"compMute",0},{"satMute",1},{"dryEqMute",1},{"output",5} } },
    };
    return bank;
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `cmake --build build --target HoldoverTests -j && ./build/tests/HoldoverTests "[presets]"`
Expected: PASS — the count test now sees 11, and "every preset references only valid parameter IDs and finite values" (which also checks non-empty and unique names) passes for both new presets, confirming every `paramID` is real and every value is finite.

- [ ] **Step 5: Run the full headless suite (no regressions)**

Run: `./build/tests/HoldoverTests`
Expected: all tests pass.

- [ ] **Step 6: Commit**

NOTE on staging: `Source/presets/FactoryPresets.cpp` and `tests/test_presets.cpp` carry pre-existing uncommitted edits (the "Character Showcase" preset, Character added to "Vocal Glue"/"Bass Saturator", and the 8 → 9 count). Staging these two whole files bundles that related preset work with the two new presets into one coherent "character presets" commit — which is intended. Do NOT stage `Source/dsp/DriveBlock.{cpp,h}` (separate DSP concern, stays uncommitted). **Confirm this bundling with the user before committing.**

```bash
git add Source/presets/FactoryPresets.cpp tests/test_presets.cpp
git commit -m "feat(presets): add Console Warmth and Bus Glue Grit character presets"
```

---

## Self-Review notes

- **Spec coverage:** Both presets from the spec are added verbatim with the exact parameter sets; the count bump (9 → 11) and reliance on the existing validity test are implemented; no DSP/UI/param changes (in scope). The commit note carries the spec's "confirm commit strategy" requirement.
- **Param validity:** every ID used (`compEngage`, `drive`, `masMode`, `curve`, `character`, `satEngage`, `hexEngage`, `eqEngage`, `filtEngage`, `satFeedLevel`, `satMute`, `compMute`, `dryEqMute`, `output`, `threshold`, `behavior`, `attack`, `release`, `makeup`, `compFeedLevel`) appears in `params::allIDs()` and is used with the correct value type (float real / choice index / 0-1 bool). The `[presets]` validity test is the guardrail if any typo slips in.
