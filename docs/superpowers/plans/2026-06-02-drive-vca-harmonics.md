# Drive + VCA Harmonics (Phase A) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add level-dependent harmonic character to Holdover — Class-A asymmetric bias in the drive stage and gain-reduction-tracking THD in the VCA compressor — behind a single defeatable `Character` control so the change can be A/B'd.

**Architecture:** Two self-contained DSP edits plus parameter wiring. `DriveBlock` gains a per-channel asymmetric DC bias injected into its shaper chain (generates 2nd-harmonic "growl", de-correlated L/R via opposite sign). `Compressor` gains an asymmetric, odd-dominant waveshaper applied after the VCA gain, scaled by instantaneous gain reduction. A new `character` APVTS parameter (0–10, default 0 = clean) threads through `ChannelStrip::Targets` to both blocks. Default 0 keeps every existing test and the shipping sound unchanged; turning it up engages the new behavior.

**Tech Stack:** JUCE 8, C++20, Catch2 tests, CMake. Both nonlinearities already live inside the oversampled region (`ChannelStrip::processOversampled`), so they are alias-safe.

**Source of truth:** `docs/superpowers/specs/2026-06-02-analog-character-study.md` §3 (Drive #1), §4 (Compressor #2), §9 (constraints). Tuning constants below are **ear-tuned guardrails, not targets** — tests assert *direction and magnitude of change*, not exact values.

---

## File Structure

- `Source/dsp/DriveBlock.h` — add `setCharacter(float amount01, float chSign)`, members `character_`, `bias_`.
- `Source/dsp/DriveBlock.cpp` — implement setter; inject `bias_` before the shaper chain.
- `Source/dsp/Compressor.h` — add `setVcaCharacter(float amount01)`, member `vcaChar_`.
- `Source/dsp/Compressor.cpp` — implement setter; add `vcaSat()` shaper applied after VCA gain, scaled by `lastReduction_`.
- `Source/dsp/ChannelStrip.h` — add `float characterPos;` to `Targets`.
- `Source/dsp/ChannelStrip.cpp` — in `applyTargets()`, normalize and fan out to `drive_[0/1].setCharacter(±)` and `comp_.setVcaCharacter()`.
- `Source/Parameters.cpp` — add `character` parameter; append to `allIDs()`.
- `Source/PluginProcessor.h` — add `smCharacter_` smoothed value.
- `Source/PluginProcessor.cpp` — init + advance `smCharacter_`; set `t.characterPos`.
- `tests/test_drive.cpp` — bias 2nd-harmonic test + per-channel de-correlation test.
- `tests/test_compressor.cpp` — THD-tracks-GR test + clean-when-no-GR test.
- `tests/test_channelstrip.cpp` — end-to-end wiring test.
- `tests/test_parameters.cpp` — add `character` to the expected ID list.

**Build/run reference (run from repo root `/Users/matt/src/plugin-holdover`):**
- Build DSP suite: `cmake --build build --target HoldoverTests`
- Run one tag: `./build/tests/HoldoverTests "[drive]"`
- Full DSP suite: `./build/tests/HoldoverTests`
- Editor suite: `cmake --build build --target HoldoverEditorTests && ./build/tests/HoldoverEditorTests`

---

## Task 1: DriveBlock Class-A asymmetric bias

**Files:**
- Modify: `Source/dsp/DriveBlock.h`
- Modify: `Source/dsp/DriveBlock.cpp`
- Test: `tests/test_drive.cpp`

- [ ] **Step 1: Write the failing tests**

Append to `tests/test_drive.cpp` (after the last test, before nothing — it's the end of file):

```cpp
TEST_CASE("Class-A bias adds 2nd-harmonic growl scaled by character", "[drive]") {
    auto secondHarmonicRatio = [](float character) {
        holdover::DriveBlock d; d.prepare(kSr); d.reset();
        d.setDrive(8.0f); d.setMas(0); d.setSat(true); d.setHex(false); d.setCurve(false);
        d.setCharacter(character, +1.0f);
        auto h = test::harmonicMagnitudes([&](float x){ return d.processSample(x); },
                                          kSr, 1000.0f, 5);
        return h[1] / h[0];   // 2nd harmonic relative to fundamental
    };
    const float clean   = secondHarmonicRatio(0.0f);  // SAT alone (odd tanh) => ~no 2nd
    const float colored = secondHarmonicRatio(1.0f);
    REQUIRE(clean < 0.01f);
    REQUIRE(colored > 0.02f);
    REQUIRE(colored > clean * 3.0f);
}

TEST_CASE("Class-A bias is opposite per channel (stereo de-correlation)", "[drive]") {
    auto captureChannel = [](float chSign, std::vector<float>& out) {
        holdover::DriveBlock d; d.prepare(kSr); d.reset();
        d.setDrive(8.0f); d.setMas(0); d.setSat(true); d.setHex(false); d.setCurve(false);
        d.setCharacter(1.0f, chSign);
        const double w = 2.0 * juce::MathConstants<double>::pi * 1000.0 / kSr;
        for (int n = 0; n < 2048; ++n)
            out.push_back(d.processSample(0.5f * (float) std::sin(w * n)));
    };
    std::vector<float> left, right;
    captureChannel(+1.0f, left);
    captureChannel(-1.0f, right);
    float maxDiff = 0.0f;
    for (size_t i = 0; i < left.size(); ++i)
        maxDiff = juce::jmax(maxDiff, std::abs(left[i] - right[i]));
    REQUIRE(maxDiff > 1.0e-3f);
}
```

`<vector>` and `<numeric>` are already available via the existing includes in this file (`#include <numeric>` is present; `std::vector` comes through `TestSignals.h`).

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target HoldoverTests`
Expected: **compile error** — `'class holdover::DriveBlock' has no member named 'setCharacter'`.

- [ ] **Step 3: Add the setter and members to the header**

In `Source/dsp/DriveBlock.h`, add the setter declaration after the `setCurve` line:

```cpp
    void setCurve(bool on) noexcept;
    void setCharacter(float amount01, float chSign) noexcept; // Class-A bias; chSign = +1/-1 per channel
```

And add the two members to the private section, immediately after the `mas_/sat_/hex_/curve_` line:

```cpp
    int mas_ = 0; bool sat_ = false, hex_ = false, curve_ = true;
    float character_ = 0.0f, bias_ = 0.0f; // Class-A asymmetry; bias_ == 0 when character_ == 0
```

- [ ] **Step 4: Implement the setter and inject the bias**

In `Source/dsp/DriveBlock.cpp`, add the setter next to the other setters (after `setCurve`):

```cpp
void DriveBlock::setCurve(bool on) noexcept { curve_ = on; }

void DriveBlock::setCharacter(float amount01, float chSign) noexcept {
    character_ = juce::jlimit(0.0f, 1.0f, amount01);
    // A small DC offset pushed into the shaper biases it off-center, generating
    // 2nd-harmonic "growl". Opposite sign per channel de-correlates L/R. Ear-tuned.
    constexpr float kBiasMax = 0.1f;
    bias_ = chSign * kBiasMax * character_;
}
```

Then modify `processSample` to inject the bias before the shaper chain. Replace the existing body:

```cpp
float DriveBlock::processSample(float x) noexcept {
    float y = x * preGain_;
    if (curve_) y = pre_.process(y);
    y = masStage(y);
    if (sat_) y = satStage(y);
    if (hex_) y = hexStage(y);
    if (curve_) y = de_.process(y);
    y *= outComp_;
    // DC blocker only when a shaper that can introduce DC/asymmetry is active.
    // With all stages off, CURVE pre/de cancel exactly => keep identity (no HPF phase shift).
    const bool stagesActive = (mas_ != 0) || sat_ || hex_;
    if (!stagesActive) return y;
    const float out = y - dcX1_ + 0.9995f * dcY1_;
    dcX1_ = y; dcY1_ = out;
    return out;
}
```

with:

```cpp
float DriveBlock::processSample(float x) noexcept {
    float y = x * preGain_;
    if (curve_) y = pre_.process(y);
    // Class-A asymmetric bias: only meaningful when a shaper follows. bias_ is 0 when
    // character is 0, so the stages-off identity path below is unchanged.
    const bool stagesActive = (mas_ != 0) || sat_ || hex_;
    if (stagesActive) y += bias_;
    y = masStage(y);
    if (sat_) y = satStage(y);
    if (hex_) y = hexStage(y);
    if (curve_) y = de_.process(y);
    y *= outComp_;
    // DC blocker (also removes the bias-induced DC, leaving the even-harmonic content).
    // With all stages off, CURVE pre/de cancel exactly => keep identity (no HPF phase shift).
    if (!stagesActive) return y;
    const float out = y - dcX1_ + 0.9995f * dcY1_;
    dcX1_ = y; dcY1_ = out;
    return out;
}
```

- [ ] **Step 5: Run the drive tests to verify they pass**

Run: `cmake --build build --target HoldoverTests && ./build/tests/HoldoverTests "[drive]"`
Expected: **PASS** — all `[drive]` tests (the 4 existing + 2 new) pass. Existing MAS/HEX/CURVE tests are unaffected because they never call `setCharacter`, so `bias_` stays 0.

- [ ] **Step 6: Commit**

```bash
git add Source/dsp/DriveBlock.h Source/dsp/DriveBlock.cpp tests/test_drive.cpp
git commit -m "$(cat <<'EOF'
feat(dsp): Class-A asymmetric bias in DriveBlock

Inject a small per-channel DC bias into the drive shaper chain, scaled by a
0..1 character amount, to generate level-dependent 2nd-harmonic growl. Opposite
sign per channel de-correlates L/R. Bias is 0 at character 0, so the stages-off
identity path and all existing harmonic tests are unchanged.

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
EOF
)"
```

---

## Task 2: Compressor VCA THD that tracks gain reduction

**Files:**
- Modify: `Source/dsp/Compressor.h`
- Modify: `Source/dsp/Compressor.cpp`
- Test: `tests/test_compressor.cpp`

- [ ] **Step 1: Write the failing tests**

In `tests/test_compressor.cpp`, add two includes at the top (after the existing `#include <cmath>`):

```cpp
#include "../tests/TestSignals.h"
#include <numeric>
```

Add this helper inside the existing anonymous `namespace { ... }` (after the `steadyGainDb` function, before the closing `}` of the namespace):

```cpp
// Total harmonic energy (2nd..5th) relative to the fundamental, for a 1 kHz tone
// driven hard enough to produce steady gain reduction.
float harmonicRatioUnderGr(bool character) {
    holdover::Compressor c; c.prepare(kSr, 512);
    c.setThreshold(-30.0f); c.setBehavior(6.0f); c.setMakeup(5.0f);
    c.setTiming(0, 0); c.setRmsMode(false);
    c.setVcaCharacter(character ? 1.0f : 0.0f);
    c.reset();
    auto h = test::harmonicMagnitudes(
        [&](float x){ float l = x, r = x; c.processStereo(l, r, x, x); return l; },
        kSr, 1000.0f, 5);
    return std::accumulate(h.begin() + 1, h.end(), 0.0f) / h[0];
}
```

Add two test cases at the end of the file:

```cpp
TEST_CASE("VCA THD adds harmonics under gain reduction when character is on", "[comp]") {
    REQUIRE(harmonicRatioUnderGr(true) > harmonicRatioUnderGr(false) * 5.0f);
}

TEST_CASE("VCA THD stays clean when there is no gain reduction", "[comp]") {
    holdover::Compressor c; c.prepare(kSr, 512);
    c.setThreshold(0.0f); c.setBehavior(6.0f); c.setMakeup(5.0f); // -6 dB tone never exceeds 0 dB
    c.setTiming(0, 0); c.setRmsMode(false); c.setVcaCharacter(1.0f); c.reset();
    auto h = test::harmonicMagnitudes(
        [&](float x){ float s = 0.5f * x, l = s, r = s; c.processStereo(l, r, s, s); return l; },
        kSr, 1000.0f, 5);
    const float ratio = std::accumulate(h.begin() + 1, h.end(), 0.0f) / h[0];
    REQUIRE(ratio < 0.01f);
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target HoldoverTests`
Expected: **compile error** — `'class holdover::Compressor' has no member named 'setVcaCharacter'`.

- [ ] **Step 3: Add the setter and member to the header**

In `Source/dsp/Compressor.h`, add the setter declaration after `setScFilter`:

```cpp
    void setScFilter(bool on) noexcept;
    void setVcaCharacter(float amount01) noexcept; // 0 = clean VCA, 1 = max GR-tracking THD
```

Add the member after `bool rmsMode_ = false, scFilterOn_ = false;`:

```cpp
    bool rmsMode_ = false, scFilterOn_ = false;
    float vcaChar_ = 0.0f; // amount of gain-reduction-tracking VCA distortion
```

- [ ] **Step 4: Implement the shaper, setter, and apply it**

In `Source/dsp/Compressor.cpp`, add a file-local shaper next to the existing `softSat` (top of the file, after the `softSat` definition):

```cpp
// Asymmetric, odd-dominant VCA waveshaper. `drive` grows with gain reduction, so the
// signal gets thicker the harder the VCA clamps. The bias term adds a touch of 2nd to
// the tanh's odd series for "bite"; subtracting tanh(b) keeps the output centered.
static inline float vcaSat(float x, float drive) noexcept {
    const float d = 1.0f + drive * 4.0f;
    const float b = 0.08f * drive;
    return (std::tanh(d * x + b) - std::tanh(b)) / d;
}
```

Add the setter near the other setters (after `setScFilter`):

```cpp
void Compressor::setScFilter(bool on) noexcept { scFilterOn_ = on; }
void Compressor::setVcaCharacter(float amount01) noexcept {
    vcaChar_ = juce::jlimit(0.0f, 1.0f, amount01);
}
```

In `processStereo`, apply the THD immediately after the VCA gain is applied. Replace this block:

```cpp
    // 5) apply VCA gain
    const float g = envGain_;
    lastReduction_ = 1.0f - g;
    lastGrDb_ = -juce::Decibels::gainToDecibels(g);
    l *= g; r *= g;

    // 6) makeup (+ soft nonlinearity past unity)
    l *= makeupGain_; r *= makeupGain_;
    if (makeupDrive_ > 0.0f) { l = softSat(l, makeupDrive_); r = softSat(r, makeupDrive_); }
```

with:

```cpp
    // 5) apply VCA gain
    const float g = envGain_;
    lastReduction_ = 1.0f - g;
    lastGrDb_ = -juce::Decibels::gainToDecibels(g);
    l *= g; r *= g;

    // 5b) VCA harmonic distortion that tracks gain reduction (thicker the harder it
    // clamps). Zero when character is off or there is no reduction. The small per-channel
    // offset de-correlates L/R; detection above stays linked, so the image is stable.
    const float grDrive = vcaChar_ * lastReduction_;
    if (grDrive > 0.0f) {
        constexpr float kVcaChOffset = 0.05f;
        l = vcaSat(l, grDrive * (1.0f + kVcaChOffset));
        r = vcaSat(r, grDrive * (1.0f - kVcaChOffset));
    }

    // 6) makeup (+ soft nonlinearity past unity)
    l *= makeupGain_; r *= makeupGain_;
    if (makeupDrive_ > 0.0f) { l = softSat(l, makeupDrive_); r = softSat(r, makeupDrive_); }
```

- [ ] **Step 5: Run the compressor tests to verify they pass**

Run: `cmake --build build --target HoldoverTests && ./build/tests/HoldoverTests "[comp]"`
Expected: **PASS** — all `[comp]` tests (5 existing + 2 new). Existing tests never call `setVcaCharacter`, so `vcaChar_` stays 0 and the VCA path is the original clean multiply.

- [ ] **Step 6: Commit**

```bash
git add Source/dsp/Compressor.h Source/dsp/Compressor.cpp tests/test_compressor.cpp
git commit -m "$(cat <<'EOF'
feat(dsp): VCA THD that tracks gain reduction in Compressor

Apply an asymmetric, odd-dominant waveshaper after the VCA gain, scaled by the
instantaneous gain reduction and a 0..1 character amount, so the signal thickens
the harder the compressor clamps. A small per-channel offset de-correlates L/R
while detection stays linked. Zero at character 0 or no GR, so existing tests and
the clean VCA path are unchanged.

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: Character parameter and wiring

**Files:**
- Modify: `Source/dsp/ChannelStrip.h`
- Modify: `Source/dsp/ChannelStrip.cpp`
- Modify: `Source/Parameters.cpp`
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`
- Test: `tests/test_parameters.cpp`, `tests/test_channelstrip.cpp`

- [ ] **Step 1: Write the failing parameter test**

In `tests/test_parameters.cpp`, add `"character"` to the `expected` StringArray (append it at the end of the brace list, after `"ceiling"`):

```cpp
        "satEngage","hexEngage","dryEqMute","compMute","satMute","ceiling","character" };
```

- [ ] **Step 2: Write the failing wiring test**

In `tests/test_channelstrip.cpp`, add this test at the end of the file:

```cpp
TEST_CASE("character parameter changes the drive output (end-to-end wiring)", "[strip]") {
    auto capture = [](float character, std::vector<float>& out) {
        holdover::ChannelStrip s; s.prepare(kSr, kBlock, 2);
        auto t = neutralTargets();
        t.drivePos = 8.0f; t.satEngage = true;   // active shaper so the bias has effect
        t.characterPos = character;
        s.setTargets(t);
        const double w = 2.0 * juce::MathConstants<double>::pi * 1000.0 / kSr;
        int phase = 0;
        juce::AudioBuffer<float> main(2, kBlock), sc(2, kBlock);
        for (int b = 0; b < 40; ++b) {
            for (int n = 0; n < kBlock; ++n, ++phase) {
                const float v = 0.5f * (float) std::sin(w * phase);
                main.setSample(0, n, v); main.setSample(1, n, v);
            }
            sc.clear(); s.processBlock(main, &sc);
            if (b >= 38) for (int n = 0; n < kBlock; ++n) out.push_back(main.getSample(0, n));
        }
    };
    std::vector<float> clean, colored;
    capture(0.0f, clean);
    capture(10.0f, colored);
    float maxDiff = 0.0f;
    for (size_t i = 0; i < clean.size(); ++i)
        maxDiff = juce::jmax(maxDiff, std::abs(clean[i] - colored[i]));
    REQUIRE(maxDiff > 1.0e-3f);
}
```

`<vector>` is available via `dsp/ChannelStrip.h` (which includes `<juce_dsp/juce_dsp.h>`); `std::abs` via the file's existing `<cmath>`.

- [ ] **Step 3: Run the tests to verify they fail**

Run: `cmake --build build --target HoldoverTests`
Expected: **compile error** — `'struct holdover::ChannelStrip::Targets' has no member named 'characterPos'` (and, once that's added, the `[params]` test would fail on the size check until the parameter is registered).

- [ ] **Step 4: Add `characterPos` to Targets**

In `Source/dsp/ChannelStrip.h`, add the field to the `Targets` struct, right after the `drivePos`/`masMode` line:

```cpp
        float drivePos; int masMode;
        float characterPos;
```

- [ ] **Step 5: Fan the character value out in applyTargets**

In `Source/dsp/ChannelStrip.cpp`, in `applyTargets()`, the drive loop currently reads:

```cpp
    for (auto& d : drive_) {
        d.setDrive(t_.drivePos); d.setMas(t_.masMode);
        d.setSat(t_.satEngage); d.setHex(t_.hexEngage); d.setCurve(t_.curve);
    }
```

Add the per-channel character fan-out immediately after that loop (the loop can't set the per-channel sign, so it's done explicitly):

```cpp
    for (auto& d : drive_) {
        d.setDrive(t_.drivePos); d.setMas(t_.masMode);
        d.setSat(t_.satEngage); d.setHex(t_.hexEngage); d.setCurve(t_.curve);
    }
    const float char01 = juce::jlimit(0.0f, 1.0f, t_.characterPos * 0.1f);
    drive_[0].setCharacter(char01, +1.0f);
    drive_[1].setCharacter(char01, -1.0f);
    comp_.setVcaCharacter(char01);
```

- [ ] **Step 6: Register the `character` parameter**

In `Source/Parameters.cpp`, add the parameter in the Drive section (after the `masMode` line):

```cpp
    // --- Drive ---
    p.push_back(std::make_unique<APF>(pid("drive"), "Drive", Range(0.0f, 10.0f, 0.01f), 5.0f));
    p.push_back(std::make_unique<APC>(pid("masMode"), "MAS", masOptions, 0));
    p.push_back(std::make_unique<APF>(pid("character"), "Character", Range(0.0f, 10.0f, 0.01f), 0.0f));
```

And append `"character"` to the `allIDs()` static list (end of the brace list, after `"ceiling"`):

```cpp
        "satEngage","hexEngage","dryEqMute","compMute","satMute","ceiling","character" };
```

- [ ] **Step 7: Add the smoothed value and wire it in the processor**

In `Source/PluginProcessor.h`, add `smCharacter_` to the linear smoothed-value declaration (append before the `;`):

```cpp
    juce::SmoothedValue<float> smGainL_, smGainR_, smInput_, smOutput_, smDrive_, smMakeup_,
        smThreshold_, smBehavior_, smDryEq_, smComp_, smSat_, smPresenceDb_, smBassDb_, smTrebleDb_,
        smCharacter_;
```

In `Source/PluginProcessor.cpp`, initialise it in `prepareToPlay` (add to the `initLin` group, e.g. right after the drive/makeup line):

```cpp
    initLin(smGainL_, "gainL");   initLin(smGainR_, "gainR");   initLin(smInput_, "input");
    initLin(smOutput_, "output"); initLin(smDrive_, "drive");   initLin(smMakeup_, "makeup");
    initLin(smCharacter_, "character");
```

And populate the target in `processBlock` (add after the `t.drivePos` / `t.makeupPos` lines):

```cpp
    t.drivePos = adv(smDrive_, p("drive"));
    t.makeupPos = adv(smMakeup_, p("makeup"));
    t.characterPos = adv(smCharacter_, p("character"));
```

- [ ] **Step 8: Build and run the affected suites**

Run: `cmake --build build --target HoldoverTests && ./build/tests/HoldoverTests "[strip],[params]"`
Expected: **PASS** — the `[params]` size/contains check passes with 39 IDs, and the `[strip]` wiring test shows the output differs between character 0 and 10.

- [ ] **Step 9: Commit**

```bash
git add Source/dsp/ChannelStrip.h Source/dsp/ChannelStrip.cpp Source/Parameters.cpp \
        Source/PluginProcessor.h Source/PluginProcessor.cpp \
        tests/test_parameters.cpp tests/test_channelstrip.cpp
git commit -m "$(cat <<'EOF'
feat: add Character parameter wiring drive bias + VCA THD

New 0-10 'character' APVTS parameter (default 0 = clean) smoothed and threaded
through ChannelStrip::Targets to the drive Class-A bias (opposite sign per
channel) and the compressor VCA THD. Default 0 keeps the shipping sound and all
existing tests unchanged; the host generic editor exposes the control for A/B.

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: Full-suite verification

**Files:** none (verification only).

- [ ] **Step 1: Build both test targets**

Run: `cmake --build build --target HoldoverTests HoldoverEditorTests`
Expected: builds with no errors.

- [ ] **Step 2: Run the full DSP suite**

Run: `./build/tests/HoldoverTests`
Expected: **All tests passed** — the prior count plus the 6 new assertions/cases (2 drive, 2 comp, 1 strip, and the updated params count). No regressions in `[filter]`, `[eq]`, `[strip]`, `[comp]`, `[drive]`, `[params]`, or hardening tags.

- [ ] **Step 3: Run the editor suite**

Run: `./build/tests/HoldoverEditorTests`
Expected: **All tests passed** (unchanged — UI was not touched).

- [ ] **Step 4: Confirm the clean default**

Reasoning check (no command): the `character` parameter defaults to 0.0, `Targets{}` zero-initialises `characterPos`, and both DSP setters produce no effect at 0 — so an existing session/preset loads bit-identical to before this change. The "default all-off routing is near unity passthrough" strip test still passing confirms this.

- [ ] **Step 5: Final commit if any verification fixups were needed**

If steps 1–3 required no changes, there is nothing to commit. If a fixup was needed, commit it:

```bash
git add -A
git commit -m "$(cat <<'EOF'
test: verification fixups for drive/VCA harmonics phase A

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
EOF
)"
```

---

## Self-Review

**Spec coverage (study §3, §4, §9):**
- §3 Drive #1 — Class-A asymmetric bias (headline): Task 1. The non-linear even→odd-vs-drive map from §3 is **intentionally deferred** (it overlaps with the existing explicit MAS off/2nd/3rd mode and needs its own design); this slice delivers the asymmetric bias the reviewer called "the single most important move." Noted here so the omission is deliberate, not a gap.
- §4 Compressor #2 — GR-tracking, asymmetric, odd-dominant VCA THD: Task 2.
- §9.1 Aliasing — both nonlinearities run inside `processOversampled`; no base-rate harmonic generation added. ✓
- §9.2 Defeatable/A-B — `character` parameter, default 0; exposed to the host generic editor. ✓ (A dedicated UI knob is out of scope for this slice.)
- §9.3 Regression anchors — default 0 preserves the unity-passthrough path; existing drive FFT tests untouched because they don't set character. ✓
- §9.4 Realtime safety — only per-sample arithmetic added; `character` is smoothed via `SmoothedValue`. ✓

**Placeholder scan:** No TBD/TODO; every code step shows complete code. Tuning constants (`kBiasMax`, `vcaSat` `d`/`b`, `kVcaChOffset`) are concrete with comments marking them ear-tunable.

**Type/name consistency:** `setCharacter(float, float)` (DriveBlock) and `setVcaCharacter(float)` (Compressor) are used identically in Task 3's `applyTargets`. `characterPos` field name matches between `ChannelStrip.h`, `ChannelStrip.cpp`, `test_channelstrip.cpp`, and `PluginProcessor.cpp`. Parameter id `"character"` matches across `Parameters.cpp`, `allIDs()`, `test_parameters.cpp`, and the processor `p("character")` lookups.

**Stereo-life scope note:** the per-channel drive bias sign and the VCA `kVcaChOffset` provide de-correlation *intrinsic to these two moves*. The broader global per-channel offset table + crosstalk (study §6) remain a separate future slice.
