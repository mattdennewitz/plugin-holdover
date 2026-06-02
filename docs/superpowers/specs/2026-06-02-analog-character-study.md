# Analog Character & Aliveness — Ideas Study

**Date:** 2026-06-02
**Status:** Exploratory study (pre-design). Findings will be implemented selectively, gated on listening results.
**Scope:** What each Holdover DSP component can learn from 80s-era gear, and how to make the
plugin sound more *alive*. This is a catalog with implementation sketches — not an approved
implementation spec. Each finding is a candidate; we build what survives an A/B test.

---

## 1. Framing

### The two priorities (chosen)
1. **Level-dependent harmonics** — tone changes with how hard a stage is driven. Not merely
   "louder = more distortion," but the harmonic *recipe shifts*: even-order warmth when gentle,
   odd-order and higher harmonics blooming as level rises.
2. **Stereo life** — L and R are never the same circuit. Small, *consistent* per-channel offsets
   (bias, trim, cutoff) de-correlate the two sides for width and depth, instead of a centered
   image that folds to mono.

Movement/drift, noise/hum, and power-supply behavior were **de-prioritized** — covered in the
optional appendix (§11), not the main findings.

### Reference lens: hybrid
Lead with the Overstayer's own voice (VCA compressor, CMOS hex-inverter fuzz, MAS "analog chain"
saturation), and **borrow specific 80s techniques** (input-transformer THD, console-EQ amp
saturation and band interaction) where they serve the two priorities.

### Root-cause diagnosis
The current DSP is **time-invariant and level-invariant in its *character***:

- Saturation stages (`DriveBlock::masStage/satStage/hexStage`) apply a fixed harmonic recipe
  regardless of how hard they're hit — `preGain_` pushes more signal in, but the *balance* of
  even vs odd is static.
- The compressor multiplies by a clean gain; it only adds harmonics when MAKEUP is pushed past
  unity (`makeupDrive_`), never as a function of gain reduction.
- `InputStage` and `OutputStage` are clean gain.
- Both stereo channels run **identical coefficients** (`filter_[0]`/`filter_[1]`, `eq_[0]`/`eq_[1]`,
  `drive_[0]`/`drive_[1]` all get the same targets), and the compressor detector is linked — so
  the two sides are mathematically identical and fold to mono.

Both priorities trace back to this. The findings below introduce *level-dependence* and
*per-channel de-correlation* where they matter most.

---

## 2. Ranked findings

| # | Component | Priority served | Impact | Cost |
|---|-----------|-----------------|--------|------|
| 1 | DriveBlock — level-dependent even/odd balance + bias | Harmonics | High | Low |
| 2 | Compressor — VCA THD that tracks gain reduction | Harmonics | High | Low |
| 3 | InputStage — transformer-style level coloration | Harmonics + stereo seed | High | Medium |
| 4 | Global stereo layer — per-channel offset table | Stereo life | Medium-high | Low-medium |
| 5 | EqBlock — saturation on boosted bands | Harmonics | Medium | Low |
| 6 | FilterBlock — level-dependent resonance + stereo cutoff | Harmonics + stereo | Medium | Low |
| 7 | Ceiling / noise / drift | (de-prioritized) | — | — (see §11) |

---

## 3. DriveBlock — level-dependent harmonic balance *(headline)*

**Now** (`Source/dsp/DriveBlock.cpp`): three fixed shapers.
- MAS mode 1 = `tanh(x·0.25) + 0.45·(xn²)` — even term is a *fixed* fraction.
- MAS mode 2 = cubic odd soft-clip.
- SAT = `tanh(1.6x)/tanh(1.6)`. HEX = hard clip `jlimit(±1, 2.5x)`.
- `outComp_` normalizes output by `1/√preGain`. DC blocker runs only when a stage is active.
The harmonic *recipe* does not move with drive, and there is no bias asymmetry.

**80s lesson (hybrid):** tube/transformer/discrete stages lead with even-order (2nd) when gently
driven and progressively add odd-order (3rd, 5th) and higher harmonics as pushed. Bias is never
perfectly centered, so a small asymmetry generates even harmonics independent of level.

**Alive move:**
- A **drive-dependent even/odd blend**: at low `preGain_`, weight the even-flavored term; as drive
  rises, crossfade toward the odd/cubic shaper and let higher harmonics climb.
- A small **bias term** added before the shaper (generates 2nd), with **opposite sign per channel**
  for stereo life.

**Sketch:**
```cpp
// DriveBlock: members set in setDrive(pos)
float bias_ = 0.0f;        // base asymmetry, scaled down as drive rises
float evenOdd_ = 0.0f;     // 0 = even-leaning, 1 = odd-leaning; = f(pos)
float chBias_ = 0.0f;      // per-channel sign offset, set by setChannelOffset()

// in masStage (and optionally satStage):
const float xb = x + bias_ + chBias_;
const float even = /* asymmetric quadratic-flavored soft clip */;
const float odd  = /* cubic / tanh soft clip */;
return juce::jmap(evenOdd_, even, odd);
```
Add `setChannelOffset(float eps)` → `chBias_ = eps`. `ChannelStrip` sets `+eps` on `drive_[0]`,
`-eps` on `drive_[1]`.

**Cost / risk:** Low code. **The FFT harmonic-content tests (`test_drive.cpp`) will move** — MAS
even-heavy, SAT tighter, HEX odd/high assertions must be re-tuned deliberately. The existing DC
blocker already handles bias-induced DC. Keep the **stages-off + unity = identity** path untouched
(the cancellation guarantee in `processSample`).

---

## 4. Compressor — VCA THD that tracks gain reduction *(headline)*

**Now** (`Source/dsp/Compressor.cpp`): clean `l *= g; r *= g;`. Harmonics only when MAKEUP > unity
(`makeupDrive_` feeds `softSat`). Detector is linked (`max(|scL|,|scR|)`). `kDetectorFeedback`
already exists for bounce.

**80s lesson (hybrid):** VCA control elements (dbx/THAT, SSL G-bus) generate THD that **rises with
control voltage — i.e. with gain reduction**. Heavier compression → more 2nd/3rd and a slight HF
softening. This *is* the "glue" and grit; it is inseparable from the act of compressing.

**Alive move:** after applying `envGain_`, add saturation scaled by **instantaneous reduction**
(`lastReduction_ = 1 - g`), independent of the makeup path. De-correlate L/R by driving each
channel's saturation slightly differently while keeping **detection linked** (image stays stable).

**Sketch:**
```cpp
// after step 5 (apply VCA gain), before/with makeup:
const float grDrive = kVcaThd * lastReduction_;          // 0 at no GR
if (grDrive > 0.0f) {
    l = softSat(l, grDrive * (1.0f + chOffset_));
    r = softSat(r, grDrive * (1.0f - chOffset_));
}
```
Optional tone extra: a one-pole LP whose cutoff drops with GR (HF dulling under heavy clamp) —
defer unless the basic THD move proves out.

**Cost / risk:** Tiny. Add a test asserting THD increases with GR and that **zero-GR stays clean**
(protects the passthrough sanity test). Detection path is unchanged, so existing detector/timing
tests hold.

---

## 5. InputStage — transformer-style level coloration

**Now** (`Source/dsp/InputStage.h`): clean per-channel gain + master fader. **Runs at base rate,
before `os_->processSamplesUp`** in `ChannelStrip::processBlock`.

**80s lesson (hybrid):** Neve/API input iron + discrete amp add low-order harmonics that bloom with
level, gentle LF saturation, a touch of HF softening — and channel trims are never identical.
Coloring the *input* seeds harmonic character into everything downstream and is the earliest place
to introduce an L/R offset.

**Alive move:** a gentle, mostly-2nd, level-dependent soft-saturation at input, plus a per-channel
bias offset (first stereo de-correlation point).

**Architectural catch (important):** because this **generates harmonics**, doing it at base rate
will **alias**. Two options:
- **(a, recommended)** Move an input "transformer" sub-stage to the **top of
  `ChannelStrip::processOversampled`** (before the filter), leaving base-rate `InputStage` as pure
  clean gain. Aliasing-safe, consistent with every other nonlinearity in the strip.
- **(b)** Keep at base rate but restrict to a very gentle 2nd-order curve — cheaper, but a
  compromise on correctness.

**Sketch (option a):** add a small `TransformerStage` (bias + asymmetric soft clip + optional gentle
LF/HF tilt) invoked per-channel at the start of `processOversampled`, parameterized by an "input
character" amount and `chOffset_`.

**Cost / risk:** Medium — touches `ChannelStrip` routing. Validate with an alias-rejection test
(harmonics above Nyquist suppressed) and confirm unity/clean path when character = 0.

---

## 6. Global stereo layer — per-channel offset table

**Now:** nonexistent. Per-channel stage *instances* exist with **identical** coefficients; comp is
linked. Result: L == R.

**80s lesson (hybrid):** no two console channels are trimmed identically — component tolerances and
trim pots leave each path slightly different. The effect is *consistent* (not random), which is why
it reads as stable width rather than noise.

**Alive move:** a single small **fixed** offset `eps` (≈0.5–2%), threaded as `+eps` to channel 0 and
`-eps` to channel 1, applied to: filter cutoff (in cents), input/drive bias, EQ coefficient
micro-shift, and VCA THD drive. Centralized in `ChannelStrip`.

**Sketch:**
```cpp
// add to InputStage / FilterBlock / EqBlock / DriveBlock / Compressor:
void setChannelOffset(float eps) noexcept;   // index 0 gets +eps, index 1 gets -eps

// ChannelStrip::applyTargets() or prepare():
constexpr float kEps = 0.012f;
filter_[0].setChannelOffset(+kEps); filter_[1].setChannelOffset(-kEps);
drive_[0].setChannelOffset(+kEps);  drive_[1].setChannelOffset(-kEps);
// (eq similarly; comp uses chOffset_ for the THD split)
```

**Cost / risk:** Low per-site, but touches several files. **Mono compatibility** is the watch-item:
offsets must be small enough that a mono fold doesn't comb audibly. Add a mono-sum sanity test
(no severe cancellation) and a stereo-de-correlation check (L and R measurably differ).

---

## 7. EqBlock — saturation on boosted bands

**Now** (`Source/dsp/EqBlock.cpp`): clean RBJ biquads, independent bands. Presence already uses
**proportional Q** (`presenceQ`: |gain| 0→Q 0.7, 12 dB→Q 3.0) — itself the classic API/SSL move, so
the EQ is already partly "80s correct."

**80s lesson (hybrid):** console EQ amplifier/inductor stages saturate, so **boosted bands gain
harmonic richness** (not just level), and bands **interact** (loading each other) rather than summing
independently.

**Alive move:** a gentle soft-saturation engaged in proportion to **boosted-band gain × signal
level** — boosting becomes *rich*, cuts stay clean. Band interaction is a deeper, optional change.

**Sketch:**
```cpp
// in processSample, after the three biquads:
float y = high_.process(mid_.process(low_.process(x)));
if (boostAmount_ > 0.0f)               // boostAmount_ = f(max boosted gain), set in setters
    y = juce::jmap(satMix_, y, softSat(y, boostAmount_));
return y;
```

**Cost / risk:** Low. Magnitude-response tests run at low level and won't trip if saturation only
engages when *hot and boosted* — verify this gating. Add a harmonic test for the boosted+hot case.

---

## 8. FilterBlock — level-dependent resonance + stereo cutoff

**Now** (`Source/dsp/FilterBlock.cpp`): TPT SVF with a **fixed** `kSatScale = 0.1f` tanh in the BP
integrator feedback (self-limits self-oscillation — already a nice nonlinearity). Both channels get
identical `g`/`k`.

**80s lesson (hybrid):** synth/console filters (SSL channel filters, Moog/Oberheim-style ladders)
have resonance grit and effective-Q that **interact with level** — push a resonant filter hot and it
gets dirtier — and stereo pairs never track identically.

**Alive move:**
- Make the feedback-saturation amount (effectively `kSatScale`) a function of **resonance and/or
  level**, so a pushed resonant peak gains grit instead of staying glassy.
- A tiny **per-channel cutoff offset** (a few cents) via `setChannelOffset`, so resonant peaks
  shimmer/widen instead of mono-locking.

**Sketch:**
```cpp
// Svf: make kSatScale a member driven by k (resonance) and/or running level.
// setHpf/setLpf: gFromFreq(freqHz * centsToRatio(chCents_), sr_)
```

**Cost / risk:** Low. The **self-oscillation onset test (`test_filter.cpp`)** must still hold, and
low-level magnitude response should be unchanged — keep the level-dependence subtle near zero.

---

## 9. Cross-cutting constraints

These apply to every finding and must be respected by any implementation:

1. **Aliasing / oversampling.** Anything that generates harmonics belongs **inside the oversampled
   region** (`ChannelStrip::processOversampled`). Drive, EQ, Filter, Compressor, and Ceiling already
   are. `InputStage`/`OutputStage` run at **base rate** — hence finding #3's recommendation to move
   input coloration inside the oversampled block.
2. **Keep "clean" reachable.** Recommend a **global `character` macro (0–1)** and/or per-feature
   engage so the plugin can still be transparent, and so each finding can be **A/B'd** during the
   listening-gated rollout. This matters specifically because we're implementing selectively based
   on results.
3. **Protect the regression anchors.** The **unity-passthrough sanity test** (input at unity, all
   stages off ≈ passthrough) is sacred — every finding must leave it intact at `character = 0`. The
   **FFT harmonic assertions** for MAS/SAT/HEX *will* shift (findings #1, #3, #5); update them
   deliberately, not accidentally.
4. **Realtime safety.** All moves are per-sample arithmetic — no allocation, locks, or logging in
   the audio path. Any new per-sample-modulated parameter must be smoothed to avoid zipper noise.

---

## 10. Suggested build order (listening-gated)

Each phase ends with an A/B against the previous state; we keep what sounds better and cut what
doesn't. The `character` macro (§9.2) exists to make this A/B trivial.

- **Phase A — prove the thesis fast:** Findings **#1 (Drive balance)** + **#2 (VCA THD)**. Both are
  cheap, both already inside oversampling, both directly serve the harmonics priority. This is the
  fastest path to hearing whether "level-dependent harmonics" delivers.
- **Phase B — broad stereo win:** Finding **#4 (global offset)** + **#6 (filter stereo cutoff)**.
- **Phase C — deeper coloration:** Finding **#3 (input transformer; routing move)** + **#5 (EQ band
  saturation)**.
- **Phase D — optional:** the appendix items (§11), only if movement/noise/sag prove desirable.

---

## 11. Optional appendix — de-prioritized dimensions

Kept for completeness; not part of the main findings. Revisit only if Phases A–C leave the sound
wanting movement, floor, or rail grind.

### 11.1 Ceiling supply sag
**Now:** `CeilingLimiter` is a hard-clip + LP "power-rail grind." **80s lesson:** clipping into the
rails with PSU sag makes the ceiling momentarily **dip after big transients** → pumping grind.
**Move:** track a slow RMS envelope; lower the clip threshold as it rises (dynamic ceiling). Low
cost, lives entirely inside `CeilingLimiter`.

### 11.2 Living noise floor
Faint **per-channel de-correlated hiss**, optional mains hum/ripple, at a very low level. Cheap;
risk is it being more gimmick than glue. Per-channel de-correlation would also reinforce stereo life.

### 11.3 Slow drift / "breathing"
Sub-Hz random-walk LFOs modulating cutoffs/bias by tiny amounts so nothing sits perfectly still.
This is the cheapest path to the *movement* dimension if we ever want it; deliberately excluded now
because movement/drift was de-prioritized.

---

## 12. Open questions for the build phase

- Magnitude of the stereo offset `eps` (mono-compatibility ceiling) — tune by ear + mono-sum test.
- Whether the `character` macro is one global control, per-feature engages, or both.
- Exact even/odd crossfade curve for Drive (finding #1) and the `kVcaThd` constant (finding #2) —
  both are ear-tuned, with FFT tests as guardrails not targets.
- Input transformer placement: confirm option (a) (move inside oversampling) over (b) (gentle
  base-rate) once we hear the difference.
