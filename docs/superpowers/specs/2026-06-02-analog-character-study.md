# Analog Character & Aliveness — Ideas Study

**Date:** 2026-06-02
**Status:** Exploratory study (pre-design). Findings will be implemented selectively, gated on listening results. Revised 2026-06-02 with DSP-engineer review refinements (see §13).
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
| 1 | DriveBlock — **Class-A** level-dependent even/odd balance + asymmetric bias | Harmonics | High | Low |
| 2 | Compressor — VCA THD that tracks gain reduction (asymmetric, odd-dominant) | Harmonics | High | Low |
| 3 | InputStage — transformer-style **LF-weighted** level coloration | Harmonics + stereo seed | High | Medium |
| 4 | Global stereo layer — per-channel offset table **+ console crosstalk** | Stereo life | Medium-high | Low-medium |
| 5 | EqBlock — saturation on boosted bands (post-band → optionally in-loop) | Harmonics | Medium | Low→Med |
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

**80s lesson (hybrid):** **Class-A** discrete stages (Neve 1073/1081) are biased toward even-order
(2nd) harmonics until they hit the rails, where odd-order and higher harmonics take over. Bias is
never perfectly centered, so a small asymmetry generates 2nd-harmonic "growl" independent of level.
*(Reviewer note: the 1073's stepped 5 dB gain also re-shapes its feedback loop per step — a
1073-specific "Zone" effect. Holdover's DRIVE is continuous, so we treat stepped feedback as an
optional flavor, not core.)*

**Alive move:**
- **Asymmetric bias is the headline** — the Class-A signature, the "Neve growl." A small `bias_`
  before the shaper generates 2nd-order content; give it **opposite sign per channel** for stereo
  life.
- A **non-linear drive→even/odd map** (not a linear crossfade): keep the sound in the **even-order
  sweet spot for roughly the first half of the `preGain_` range**, then bloom into odd-order /
  hex-inverter grit only when pushed into the "red." This matches how a Class-A stage stays warm
  until it's slammed.

**Sketch:**
```cpp
// DriveBlock: members set in setDrive(pos)
float bias_    = 0.0f;     // base asymmetry (2nd-harmonic), eased down as drive rises
float evenOdd_ = 0.0f;     // 0 = even sweet spot, 1 = odd/grit; NON-LINEAR f(pos)
float chBias_  = 0.0f;     // per-channel sign offset, set by setChannelOffset()

// evenOdd_ stays ~0 through the lower drive range, then climbs steeply:
//   evenOdd_ = smoothstep(threshold≈0.5, 1.0, normalizedDrive);   // tune by ear
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

**80s lesson (hybrid):** VCA control elements (dbx/THAT, SSL G-bus, Neve 33609 — the "British bus"
sound) generate THD that **rises with control voltage — i.e. with gain reduction**. The signal gets
*thicker* the harder the VCA clamps; under heavy GR these circuits push into a region where **3rd
(odd) harmonics dominate**, giving the aggressive "bite." This *is* the glue and grit — inseparable
from the act of compressing.

**Alive move:** after applying `envGain_`, add saturation scaled by **instantaneous reduction**
(`lastReduction_ = 1 - g`), independent of the makeup path. Make the shaper **asymmetric and
odd-leaning** so heavy GR adds bite, not just warmth. De-correlate L/R by driving each channel's
saturation slightly differently while keeping **detection linked** (image stays stable).

**Sketch:**
```cpp
// after step 5 (apply VCA gain), before/with makeup:
const float grDrive = kVcaThd * lastReduction_;          // 0 at no GR
if (grDrive > 0.0f) {
    // asymmetric, odd-dominant shaper (bias the curve so 3rd climbs under heavy GR):
    l = vcaShape(l, grDrive * (1.0f + chOffset_));
    r = vcaShape(r, grDrive * (1.0f - chOffset_));
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
level, and — critically — **transformers saturate more at low frequencies**. Core flux is inversely
proportional to frequency (Φ ∝ V/f), so for a given level the lows demand the most flux and reach
saturation first. This frequency-dependence *is* the classic Neve "low-end bloom/weight"; a flat
`tanh` misses it entirely. Real iron also has **hysteresis** (a magnetic "memory") that adds a
subtle phase shift and its own harmonic signature. And channel trims are never identical. Coloring
the *input* seeds harmonic character into everything downstream and is the earliest L/R offset point.

**Alive move:**
- A gentle, mostly-2nd, level-dependent **frequency-weighted** soft-saturation: drive the
  nonlinearity *harder in the lows* so the bottom blooms first (e.g. a low-shelf lift feeding the
  shaper, compensated after). This is the single most distinctive Neve trait to capture here.
- A per-channel bias offset (first stereo de-correlation point).
- **Hysteresis** (phase memory + its harmonic signature) is a meaningfully bigger lift than a
  frequency-weighted shaper — flagged as an **optional deeper move**, not part of the headline.

**Architectural catch (important):** because this **generates harmonics**, doing it at base rate
will **alias**. Two options:
- **(a, recommended)** Move an input "transformer" sub-stage to the **top of
  `ChannelStrip::processOversampled`** (before the filter), leaving base-rate `InputStage` as pure
  clean gain. Aliasing-safe, consistent with every other nonlinearity in the strip.
- **(b)** Keep at base rate but restrict to a very gentle 2nd-order curve — cheaper, but a
  compromise on correctness.

**Sketch (option a):** add a small `TransformerStage` invoked per-channel at the start of
`processOversampled`, parameterized by an "input character" amount and `chOffset_`. The key detail is
**LF-weighted drive** — saturate the lows harder:
```cpp
// frequency-weight the drive into the shaper so lows saturate first (Φ ∝ V/f):
const float driven = lfShelf_.process(x) * inputDrive_;   // low-shelf lifts LF into the curve
float y = asymSoftClip(driven + bias_ + chBias_);         // mostly-2nd, asymmetric
y = lfShelfInv_.process(y);                                // compensate the pre-emphasis
// optional deeper move: replace asymSoftClip with a hysteresis model (phase memory).
```

**Cost / risk:** Medium — touches `ChannelStrip` routing. Validate with an alias-rejection test
(harmonics above Nyquist suppressed) and confirm unity/clean path when character = 0.

---

## 6. Global stereo layer — per-channel offsets + console crosstalk

**Now:** nonexistent. Per-channel stage *instances* exist with **identical** coefficients; comp is
linked. Result: L == R.

**80s lesson (hybrid):** two effects, both consistent (not random), which is why they read as stable
width and glue rather than noise:
1. **Trim mismatch** — no two console channels are trimmed identically; component tolerances and
   trim pots leave each path slightly different.
2. **Channel crosstalk** *(reviewer addition, high value)* — physically adjacent channel strips bleed
   a tiny, filtered amount of each side into the other. This creates a **phantom center** and *glues*
   the stereo image. For "stereo life" specifically, crosstalk does **more than trim offsets alone**,
   because it simulates the physical proximity of the strips rather than just detuning them.

**Alive move:**
- **6a — per-channel offset.** A single small **fixed** offset `eps`, threaded as `+eps` to channel 0
  and `-eps` to channel 1, applied to: filter cutoff (in cents), input/drive bias, EQ coefficient
  micro-shift, and VCA THD drive. Centralized in `ChannelStrip`. **Keep it small (0.5–1.0%)** — a
  ~1.2% filter-cutoff offset is *audible*; ~0.5% is *felt*. Mono-compatibility sets the ceiling.
- **6b — console crosstalk.** A fixed, very low-level bleed (**≈ −80 to −60 dB**) of each channel into
  the other, gently high-shelved (the bleed is brighter / HF-tilted), summed at a single stereo point
  (output or matrix bus). Cheap and arguably the bigger stereo-life win.

**Sketch:**
```cpp
// 6a — add to InputStage / FilterBlock / EqBlock / DriveBlock / Compressor:
void setChannelOffset(float eps) noexcept;   // index 0 gets +eps, index 1 gets -eps
// ChannelStrip:
constexpr float kEps = 0.006f;               // ~0.5%, "felt" not "heard"
filter_[0].setChannelOffset(+kEps); filter_[1].setChannelOffset(-kEps);
drive_[0].setChannelOffset(+kEps);  drive_[1].setChannelOffset(-kEps);
// (eq similarly; comp uses chOffset_ for the THD split)

// 6b — crosstalk at a single stereo summing point (e.g. just before OutputStage):
const float bleedL = xtalkShelfL_.process(outR) * kXtalk;   // kXtalk ≈ 1e-3 (-60 dB)
const float bleedR = xtalkShelfR_.process(outL) * kXtalk;   // shelves tilt the bleed bright
outL += bleedL; outR += bleedR;
```

**Cost / risk:** Low per-site, but 6a touches several files; 6b needs a stereo summing point (the
channels stop being fully independent there — fine, but note it). **Mono compatibility** is the
watch-item for both: offsets/bleed must be small enough that a mono fold doesn't comb audibly. Add a
mono-sum sanity test (no severe cancellation) and a stereo-de-correlation check (L and R measurably
differ).

---

## 7. EqBlock — saturation on boosted bands

**Now** (`Source/dsp/EqBlock.cpp`): clean RBJ biquads, independent bands. Presence already uses
**proportional Q** (`presenceQ`: |gain| 0→Q 0.7, 12 dB→Q 3.0) — itself the classic API/SSL move, so
the EQ is already partly "80s correct."

**80s lesson (hybrid):** console EQ amplifier/**inductor** stages saturate, so **boosted bands gain
harmonic richness** (not just level), and bands **interact** (loading each other) rather than summing
independently. *(Reviewer detail: Neve mid-bands (1073/1081) use inductors, whose **permeability
shifts as they saturate** — so the band's center frequency and Q drift slightly under drive, not
just its harmonic content.)*

**Alive move (two tiers):**
- **Cheap win (post-band):** a gentle soft-saturation engaged in proportion to **boosted-band gain ×
  signal level** — boosting becomes *rich*, cuts stay clean. High value, low cost.
- **Take it further (in-loop):** place the saturation **inside the biquad feedback** (or simulate it
  there) so it affects the filter's **phase response** — this is what gives Neve inductor EQs their
  "smeared," musical quality, and is also where the permeability-driven Fc/Q drift would live. Higher
  complexity and risk (in-loop nonlinearity can affect stability) — a deliberate second step, not the
  first.

**Sketch (cheap win):**
```cpp
// in processSample, after the three biquads:
float y = high_.process(mid_.process(low_.process(x)));
if (boostAmount_ > 0.0f)               // boostAmount_ = f(max boosted gain), set in setters
    y = juce::jmap(satMix_, y, softSat(y, boostAmount_));
return y;
```

**Cost / risk:** Post-band tier is Low. Magnitude-response tests run at low level and won't trip if
saturation only engages when *hot and boosted* — verify this gating; add a harmonic test for the
boosted+hot case. The in-loop tier is Medium — needs a stability check (no runaway in the saturated
feedback) and will alter the EQ's phase/magnitude under drive by design.

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

There are **two defensible orderings** — the choice is cost-first vs impact-first:

- **Cost-first (original):** start with the near-free moves to prove the thesis fastest.
  1. Drive (#1) + VCA THD (#2) — both already inside oversampling, both directly serve harmonics.
  2. Global stereo (#4: offsets + crosstalk) + filter stereo cutoff (#6).
  3. Input transformer (#3, routing move) + EQ saturation (#5).
  4. Optional appendix (§11).
- **Impact-first (reviewer verdict):** lead with the moves the DSP review rates highest-impact,
  accepting the input-stage routing cost up front.
  1. **Input "iron" inside oversampling, LF-weighted (#3)** — rated the most distinctive Neve trait.
  2. **Asymmetric Class-A bias in Drive (finding #1)** — the Class-A signature.
  3. VCA THD (#2), then stereo (#4) with **small** offsets (0.5–1.0%) for mono safety.
  4. EQ saturation (#5, post-band first), filter (#6), optional appendix (§11).

**Tradeoff to decide:** Input Iron (#3) is arguably the biggest *sonic* win but carries the
routing-move cost (§5 option a); Drive+VCA were Phase A precisely because they're *near-free to try*.
If the goal is to validate the whole thesis with minimal code, go cost-first; if you trust the
review and want the strongest result soonest, go impact-first. Either way each step is A/B-gated.

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

- Build ordering: cost-first vs impact-first (§10) — the one decision that shapes the whole rollout.
- Magnitude of the stereo offset `eps` (mono-compatibility ceiling) — start ~0.5%, tune by ear +
  mono-sum test. And crosstalk level `kXtalk` (−80…−60 dB) + its shelf tilt.
- Whether the `character` macro is one global control, per-feature engages, or both.
- Drive (#1): the **non-linear** even→odd map shape (even sweet-spot threshold) and `bias_` amount;
  and the `kVcaThd` constant + asymmetry for the VCA (#2). All ear-tuned, FFT tests as guardrails.
- Input transformer (#3): confirm option (a) (inside oversampling) over (b); LF-weighting corner +
  amount; whether hysteresis is worth the extra lift.
- EQ (#5): stop at the post-band cheap win, or invest in the in-loop / inductor-permeability tier?

---

## 13. Reviewer refinements incorporated (2026-06-02)

DSP-engineer review of the original study; changes folded into the findings above:

- **#3 Input** — added **LF-weighted saturation** (Φ ∝ V/f → lows bloom first; the defining Neve
  trait). **Hysteresis** noted as an optional deeper move, not headline.
- **#1 Drive** — reframed around **Class-A asymmetric bias** as the headline ("Neve growl"), and made
  the even→odd map **non-linear** (even sweet-spot through the lower drive range, odd bloom only in
  the red). Stepped-feedback "Zone" noted as optional, non-core flavor.
- **#5 EQ** — added the **in-loop saturation** tier (affects phase → the "smeared" inductor quality)
  above the cheap post-band win, plus inductor **permeability-driven Fc/Q drift**.
- **#4 Stereo** — promoted **console crosstalk** (−80…−60 dB, HF-tilted, summed at one stereo point)
  as a primary stereo-life technique alongside trim offsets; **reduced the offset magnitude to
  0.5–1.0%** (1.2% is audible; 0.5% is "felt").
- **#2 Compressor** — specified the VCA THD shaper as **asymmetric and odd-dominant** under heavy GR
  (the "British bus" bite), not just symmetric warmth.
- **§10 Build order** — added an **impact-first** ordering (reviewer verdict: Input Iron #1, Class-A
  bias #2) beside the original cost-first ordering, with the tradeoff made explicit.

Assessment from the review: a "9/10 roadmap"; the highest-leverage additions are LF-weighted input
saturation and asymmetric Class-A drive bias.
