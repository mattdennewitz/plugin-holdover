# Holdover — Design Spec

**Date:** 2026-06-02
**Status:** Approved design, pre-planning
**Working name:** Holdover

A character-faithful JUCE plugin (VST3 + AU, macOS only) modeled on the Overstayer
Modular Channel (8755DS/8755DM). Original DSP that captures the unit's sonic
behavior and routing — not a circuit-accurate clone, and not a generic strip.

---

## 1. Goals & scope

### In scope
- Stereo + mono channel strip: resonant filters → 3-band EQ → VCA compressor with
  the BEHAVIOR envelope → three cascadable saturation stages (MAS/SAT/HEX) →
  series/parallel feed matrix (DRY/EQ, COMP, SAT) → ceiling limiter → output.
- External **sidechain** for the compressor detector (host sidechain bus).
- Linked stereo compressor detection (stable image/glue); per-channel saturation.
- Factory preset bank + host preset save/load.
- Resizable, modern flat UI (dark theme).
- macOS universal binary (arm64 + x86_64), formats VST3 + AU.

### Out of scope (hardware-only or deferred)
- Input selection (mic / instrument / line), phantom power, the "floating" switch,
  balanced I/O routing, the hardware INSERT send/return.
- Polarity (POL L/R) switches.
- A/B compare snapshots.
- Windows / Linux; AAX.

### Fidelity target
Character-faithful. We have no schematics, so we build original DSP that reproduces
the described and reviewed *behavior*: harmonic content per stage, the BEHAVIOR
envelope feel, self-oscillating resonant filters, proportional-Q presence band, and
the parallel feed matrix. We do not attempt SPICE-level matching or measured-capture
modeling.

---

## 2. Verified signal flow

Confirmed against the 8755D block diagram and the v2.1 reference manual.

Serial drive path:

```
INPUT (GAIN L/R trim → INPUT fader) → [FILTERS] → [EQ] → [COMP] → DRIVE → MAS → SAT → HEX → SAT feed
```

Three parallel feeds, each with its own level + mute, summed at the matrix bus,
then through the ceiling limiter to the output:

```
DRY/EQ feed = pre-filter (input fader) OR post-EQ
COMP feed   = compressor output (always, regardless of COMP-in-path switch)
SAT feed    = full wet path (post-HEX)

matrix = Σ(feed · level · !mute) → CEILING → OUTPUT
```

### Switch routing (faithful behavior)
The FILT / EQ / COMP switches insert each block into the serial drive path. When a
block is switched **out**, it leaves the serial path; EQ and COMP are then fed
directly from the input fader and contribute **only** through their parallel feeds.
DRIVE is always active (its harmonic stages are individually optional).

Precise model (per sample/block):

```
inputFaderOut = inputStage(in)                       // GAIN L/R + INPUT
filtOut       = FILT_on ? filter(inputFaderOut) : inputFaderOut
eqIn          = EQ_on   ? filtOut : inputFaderOut
eqOut         = eq(eqIn)                              // always computed
serialAfterEq = EQ_on   ? eqOut : filtOut
compIn        = COMP_on ? serialAfterEq : inputFaderOut
compOut       = comp(compIn, scDetector)             // always computed
serialAfterComp = COMP_on ? compOut : serialAfterEq
driveOut      = drive(serialAfterComp)               // MAS→SAT→HEX

dryEqFeed = (dryEqSource == PRE) ? inputFaderOut : eqOut
compFeed  = compOut
satFeed   = driveOut

scDetector source = (scSource == PRE) ? inputFaderOut
                  : (scSource == POST) ? eqOut
                  : externalSidechainBus              // EXT (middle)
```

If EXT sidechain is selected but no bus is connected, the detector sees silence and
compression is effectively bypassed (faithful to hardware).

---

## 3. DSP modules

Each module is a self-contained C++ class with no JUCE-processor coupling, so it can
be unit-tested headless. All are realtime-safe: no allocation, locks, or logging in
the audio path; parameter changes are smoothed.

### 3.1 InputStage
- **GAIN L / GAIN R** — independent per-channel input trim (adaptation of the
  hardware preamp gain; the plugin has no preamp). Maps 0–10 → dB range (TBD in
  plan, e.g. −∞/−24…+24 dB).
- **INPUT** — master input fader, unity at 5.

### 3.2 FilterBlock
- **HPF**: frequency 20 Hz – 4.7 kHz; **PEAK** = resonance, self-oscillates above 8.
- **LPF**: frequency 220 Hz – 22 kHz; **PEAK** = resonance, self-oscillates above 8.
- Implementation: TPT/SVF (Zavalishin-style) state-variable filters. A saturating
  nonlinearity (tanh/soft-clip) sits **inside the resonance feedback path** so that
  self-oscillation self-limits to a fixed amplitude (no digital runaway) and gains
  musical grit when pushed. This filter-loop nonlinearity is distinct from the
  DriveBlock saturation.
- **CURVE** switch — pre/de-emphasis around the saturation stages that preserves
  low-end power under drive (default ON); OFF = looser, dirtier lows. Functionally
  a drive-stage control; placed in the UI near Drive (hardware silkscreens it in the
  bandwidth section). Single CURVE control. **Baseline: pre- and de-emphasis are
  mathematical inverses**, so with the stages undriven the frequency response is flat
  and CURVE only manifests through the nonlinearity (expected: inaudible at zero
  drive). A slight intentional non-inverse coloration may be allowed as a tuning call.

### 3.3 EqBlock
- **Low shelf**: ±18 dB, frequency switch 50 / 150 / 300 Hz.
- **Presence (mid bell)**: ±12 dB, sweepable ~260 Hz – 12 kHz, **proportional Q**
  (Q tightens as boost/cut magnitude increases).
- **High shelf**: ±15 dB, frequency switch 5 / 10 / 15 kHz.
- Implementation: TPT/RBJ biquads. Proportional-Q maps |gain| → Q.

### 3.4 Compressor (VCA, feed-forward)
- **No ratio control.** THRESHOLD + BEHAVIOR define the compression.
- **THRESHOLD** — onset level. Pull-switch → **RMS** detection (disables ATTACK /
  RELEASE).
- **BEHAVIOR** — nonlinearly magnifies the gain-reduction curve and dynamically
  skews attack/release timing. At 0: gentle, set timing. As increased: steeper
  effective ratio, faster/more reactive timing, eventually musical over-compression
  ("active, bouncing" feel). Implementation: **program-dependent timing** — the
  effective attack/release constants are a log-domain interpolation between the
  fast/slow values keyed to the *instantaneous* gain-reduction amount (not the raw
  parameter), which produces the clutch/bounce. BEHAVIOR scales how strongly GR skews
  both ratio and that timing. An optional small detector-feedback term may be added
  for extra "bounce." The unit is a feed-forward "clean VCA," so feedback is a
  character refinement, not the base topology. Tuned by ear in prototyping.
- **ATTACK / RELEASE** — 3-position each (Fast / Medium / Slow), discrete time
  constants. Disabled in RMS mode.
- **MAKEUP** — unity at 5. Pushed past unity, a soft nonlinearity is added (the
  "clean VCA distorts beautifully"). Pull-switch → **SC FILTER** (high-pass on the
  detector signal, so the detector sees less low end).
- **SC source** — 3-position: PRE (pre-filter) / EXT (external bus) / POST (post-EQ).
- **Stereo**: linked detector (max/sum of channels) so both channels share gain
  reduction.

### 3.5 DriveBlock (oversampled)
- **DRIVE** — single drive control feeding the cascade; unity at 5; always active.
- Stages cascade **MAS → SAT → HEX** (highest → lowest headroom):
  - **MAS** — 3-position (off / 2nd-emphasis / 3rd-emphasis): even+odd low-order
    harmonics with soft peak-rounding (vintage "analogue chain" character).
  - **SAT** — on/off: tight saturation, clips sooner, reaches into the lower-mids
    without adding murk (tanh/diode-style waveshaper, tighter knee).
  - **HEX** — on/off: aggressive CMOS hex-inverter fuzz; hard clipping rich in
    high-order content, grinds the top end, does not "clean up."
- **Oversampling**: required because the SAT/HEX hard nonlinearities alias heavily.
  Architecture (drive-only vs whole-matrix; FIR linear-phase vs IIR min-phase) is the
  one open decision — see §6 and §9. Whatever is chosen, the resulting latency is
  reported to the host and the parallel feeds are delay-compensated (see FeedMatrix).
- **CURVE** emphasis applied around this block (see 3.2).

### 3.6 FeedMatrix
- Sums the three feeds, each with a continuous level and a mute:
  **DRY/EQ feed** (+ source switch PRE/POST), **COMP feed**, **SAT feed**.
- **Owns summing + phase coherence** as the central "bus matrix." With the chosen
  whole-matrix oversampling (§6), all three feeds run at the high rate and share one
  latency, so no per-feed delay lines are needed and toggling drive stages never
  introduces comb filtering. The matrix reports the single shared latency to the host
  via `setLatencySamples`. (If a future build moves to drive-only oversampling, this
  class is where per-feed delay compensation would live.)

### 3.7 CeilingLimiter
- After the matrix bus, before the output fader. **Not** a transparent lookahead
  limiter — to capture the "power-rail" grind, implement as a hard-clipper / shaper
  followed by a fast-release low-pass (waveshaper character), so it grinds the
  waveform toward a near-static, distorted state when pushed. Engaged via the
  **CEILING** switch. Because it hard-clips, it must sit **inside the oversampled
  region**.

### 3.8 OutputStage
- **OUTPUT** — master output fader.

### 3.9 Metering
- Two LED-ladder meters rendered flat: **SATURATING** (drive activity) and
  **COMPRESSING** (gain reduction). Plus a small numeric GR readout. Meter values
  produced by the DSP, consumed by the UI via an atomic/lock-free path.

---

## 4. Parameters (APVTS)

All parameters live in an `AudioProcessorValueTreeState`. Hardware 0–10 scales map to
meaningful internal units; switch positions become choice/bool params.

**Continuous:** gainL, gainR, input, hpfFreq, hpfPeak, lpfFreq, lpfPeak, presenceDb,
presenceFreq, bassDb, trebleDb, threshold, behavior, makeup, drive, dryEqFeedLevel,
compFeedLevel, satFeedLevel, output.

**Choice:** bassFreq (50/150/300), trebleFreq (5k/10k/15k), attack (F/M/S),
release (F/M/S), masMode (off/2nd/3rd), scSource (pre/ext/post), dryEqSource (pre/post).

**Bool / toggle:** filtEngage, eqEngage, compEngage, curve, rmsMode, scFilter,
satEngage, hexEngage, dryEqMute, compMute, satMute, ceiling.

Exact ranges, skews, and default values are fixed in the implementation plan.

---

## 5. UI (modern flat — direction B)

- Dark theme, flat controls (knobs, sliders, toggles). Controls grouped into labeled
  panels following signal order: **Input · Filters · EQ · Compressor · Drive ·
  Matrix/Output**.
- The two LED-ladder meters sit near the Compressor and Drive sections.
- Resizable window (constrained aspect / min size). No reliance on the hardware
  faceplate trade dress.
- UI reads/writes parameters via APVTS attachments; meter display via lock-free
  values from the processor.

---

## 6. Tech stack & build

- **JUCE 8.x**, C++20, **CMake** (`juce_add_plugin`).
- `juce_dsp` for oversampling and filter primitives where useful.
- Formats: **VST3 + AU**. Platform: **macOS only**, universal binary (arm64 +
  x86_64).
- Sidechain: declare a sidechain input bus; `isBusesLayoutSupported` accepts
  mono/stereo main + optional sidechain.
- State: APVTS `getStateInformation` / `setStateInformation`. Presets: factory bank
  (XML/binary embedded) + user presets via host.
- Realtime discipline: no allocation/locks in `processBlock`; oversampling buffers
  preallocated in `prepareToPlay`.
- Parameter smoothing via `juce::SmoothedValue` with ~5–10 ms ramps to kill zipper
  noise during automation/"played" use. Frequency parameters (filter cutoffs,
  presence freq) smoothed **multiplicatively** (log domain), not linearly.
- **Oversampling architecture (decided):** oversample the *entire matrix region once*
  — all three feeds (DRY/EQ, COMP, SAT) run at the high rate and are downsampled once
  at the output. All feeds therefore share identical latency (phase-coherent by
  construction, no per-feed delay lines needed), and minimum-phase oversampling is
  usable for transient feel. Factor (4× vs 8×) is the remaining tuning knob.
- Latency: reported via `setLatencySamples` (single, shared across feeds).

---

## 7. Testing

- **DSP core decoupled** from the JUCE processor → fast headless unit tests
  (Catch2 or JUCE UnitTest).
- Coverage:
  - Filter & EQ magnitude response at known settings; self-oscillation onset.
  - Presence proportional-Q behavior (Q vs gain).
  - Compressor static curve; RMS vs peak; attack/release timing; sidechain source
    selection; SC filter effect.
  - Saturation stages: **FFT harmonic-content assertions** (MAS even/odd low-order;
    SAT tighter spectrum; HEX high-order/odd-heavy) and alias-rejection check with
    oversampling on.
  - Feed matrix summing and mute behavior; switch re-route behavior (EQ/COMP out →
    parallel-only).
  - Bypass / unity sanity (input at unity, all stages off ≈ passthrough).
  - Parameter ranges / no NaNs / denormal handling.
- **pluginval** (strict) for format/host validation.

---

## 8. Implementation phasing (for the plan)

1. Scaffold: CMake + JUCE, plugin targets, APVTS, passthrough, sidechain bus, CI build.
2. Filters + EQ (+ tests).
3. Compressor incl. BEHAVIOR, detection modes, sidechain (+ tests).
4. Drive stages MAS/SAT/HEX + oversampling + CURVE (+ FFT tests).
5. Feed matrix + ceiling + output + switch re-route logic (+ tests).
6. UI (modern flat), meters, parameter attachments, resizing.
7. Factory presets + state persistence.
8. pluginval, performance pass, denormal/edge hardening, polish.

---

## 9. Open items to resolve in the plan

- Oversampling **factor** (4× vs 8×) and an optional quality switch. (Architecture is
  decided — see §6.)
- Exact dB/Hz mappings for the 0–10 hardware scales (GAIN, INPUT, OUTPUT, feeds,
  DRIVE, MAKEUP).
- BEHAVIOR detail: timing-interpolation curve, GR→ratio skew strength, and whether
  the optional detector-feedback term is included.
- Whether CURVE is one switch (assumed) or separate HPF/LPF emphasis switches.
- Factory preset list and starting values.
