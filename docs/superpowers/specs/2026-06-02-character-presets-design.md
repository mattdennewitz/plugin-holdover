# Character-Isolating Factory Presets — Design

**Date:** 2026-06-02
**Status:** Approved

## Goal

Add two factory presets that make the **Character** macro apparent, obvious, and
useful — each isolating one of Character's two independent mechanisms in a real,
reach-for-it musical patch.

## Background: what Character does

Character (param `character`, float 0–10) is wired in `ChannelStrip::applyTargets`
(`char01 = characterPos * 0.1`) to two places:

1. **Drive bias** — `drive_[0/1].setCharacter(char01, ±1)` pushes a DC offset into
   the Drive shaper, generating 2nd-harmonic "Class-A growl" (opposite sign per
   channel for stereo width). It only colors the signal when a shaper is active
   (`mas_ != 0 || sat_ || hex_`); it is cleanest/warmest with **MAS mode 1**
   (2nd-emphasis) and is masked by the SAT/HEX hard-clip stages.
2. **Comp VCA THD** — `comp_.setVcaCharacter(char01)` scales harmonic distortion
   by instantaneous gain reduction (`grDrive = vcaChar_ * lastReduction_`). It only
   blooms when the compressor is actually clamping (low threshold + signal driving GR).

The serial chain is **in → filter → EQ → comp → drive**. The output matrix taps
three points (`FeedMatrix::mix`): **dryEq** (clean), **comp feed** (= comp output,
carries only the VCA THD), **sat feed** (= drive output, carries VCA THD *plus* the
drive bias). The existing (uncommitted) "Character Showcase" preset maxes both
mechanisms at once as an A/B demo; these two presets instead isolate each one.

## The two presets

Both disable EQ and filter and route a single matrix feed, so Character is the only
thing coloring the audible signal. Loading either and sweeping Character 0 → 8 makes
its contribution unmistakable, and each is a usable patch on its own.

### A. "Console Warmth" — isolates the Drive 2nd-harmonic bias

Compressor off; MAS-1 shaper on; route the sat feed (drive output). Character becomes
a pure even-harmonic analog-warmth knob. Useful on a DI, synth, or mix bus.

```
compEngage 0, drive 5, masMode 1, curve 1, character 8,
satEngage 0, hexEngage 0, eqEngage 0, filtEngage 0,
satFeedLevel 10, satMute 0, compMute 1, dryEqMute 1, output 5
```

Rationale: with the compressor off there is no VCA THD and no squash; with SAT/HEX
off the even harmonics aren't masked; the only thing in the audible (sat-feed) path
is the biased MAS-1 shaper. Character 0 = clean, 8 = warm growl.

### B. "Bus Glue Grit" — isolates the Comp VCA THD

Fast bus compression with real gain reduction; no drive shaper (MAS off → Drive is
transparent); route the comp feed. Character becomes the THD that blooms as the VCA
clamps — the "British bus" bite. Useful on a drum or mix bus.

```
compEngage 1, threshold -22, behavior 5, attack 1, release 2, makeup 7,
masMode 0, satEngage 0, hexEngage 0, character 8,
eqEngage 0, filtEngage 0,
compFeedLevel 10, compMute 0, satMute 1, dryEqMute 1, output 5
```

Rationale: with `masMode 0` (and SAT/HEX off) the Drive block is identity, so the
compressor is the only harmonic source; the comp feed carries `grDrive = character ×
gainReduction`. Character 0 = clean glue, 8 = gritty bite tracking how hard it works.

## Parameter validity

All IDs and value types are verified against `Source/Parameters.cpp`:
- Floats (real values): `drive`, `character`, `threshold`, `behavior`, `makeup`,
  `output`, `*FeedLevel`.
- Choice indices: `masMode` (0=off,1=2nd,2=3rd), `attack`, `release`.
- Bool toggles (0/1): `compEngage`, `curve`, `satEngage`, `hexEngage`, `eqEngage`,
  `filtEngage`, `satMute`, `compMute`, `dryEqMute`.

The preset `apply()` path converts these real values to normalized and notifies the
host; out-of-list params keep their defaults.

## Implementation

- Append both presets to the bank in `Source/presets/FactoryPresets.cpp` (after the
  existing "Character Showcase").
- Update the count assertion in `tests/test_presets.cpp` from 9 to **11**. The
  existing `[presets]` test "every preset references only valid parameter IDs and
  finite values" (which also asserts non-empty, unique names) then validates both new
  presets automatically — no new test case needed.

## Out of scope

No DSP changes (Character's behavior and the deepened `kBiasMax` bias are pre-existing
branch work). No UI changes. No new parameters. Renaming or removing existing presets.

## Commit note

These additions sit on top of uncommitted branch work (the `DriveBlock` bias
deepening + smoothing and the existing showcase/preset edits). Only the preset and
test changes are in scope here; the commit strategy relative to the pre-existing
uncommitted DSP edits is confirmed with the user at implementation time.
