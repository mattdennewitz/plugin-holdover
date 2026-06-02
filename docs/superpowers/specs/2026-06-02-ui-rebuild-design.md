# Holdover — UI Rebuild Design Spec

**Date:** 2026-06-02
**Status:** Approved design, pre-planning
**Scope:** UI relayout + restructure only. No DSP, parameter, or preset changes.

A rebuild of the Holdover editor into a clean, strictly-aligned **two-column console**
layout. Same controls, same parameters, same dark flat theme — only the arrangement,
component styling, and sizing change. Validated through browser mockups
(`.superpowers/brainstorm/.../console-v2.html`).

---

## 1. Goals

- **Strict alignment.** Every knob is the same diameter; knob columns line up
  vertically down each side; both columns bottom-align; gutters are uniform.
- **Proportional resize.** The whole UI is laid out once at a fixed base size and
  scaled *uniformly* with a locked aspect ratio. Nothing ever stretches or reflows.
- **Obvious affordances.** Engage/bypass controls read unmistakably as switches;
  mode toggles read as lit buttons.
- **Opens at full size.** A fresh instance opens at the base design size, not a
  shrunk-down window.

### Out of scope
- DSP, signal flow, parameter ranges/IDs, factory presets — all unchanged.
- New controls or removed controls. The control inventory is identical to today.
- Theme palette change (colors stay; component *shapes* change).

---

## 2. Layout

Three regions side by side inside a 12px outer margin, 12px gutters:

```
┌─────────────────────────────────────────────────────────────┐
│  HOLDOVER                                    Modular Channel  │  header (36px)
├──────────────────────┬─────────┬────────────────────────────┤
│  LEFT — tone shaping │ METERS  │  RIGHT — dynamics & output  │
│  ┌────────────────┐  │ ┌─────┐ │  ┌──────────────────────┐  │
│  │ INPUT          │  │ │ S C │ │  │ COMPRESSOR           │  │
│  ├────────────────┤  │ │ A O │ │  ├──────────────────────┤  │
│  │ FILTERS        │  │ │ T M │ │  │ DRIVE                │  │
│  ├────────────────┤  │ │ ▮ ▮ │ │  ├──────────────────────┤  │
│  │ EQ             │  │ │ ▮ ▮ │ │  │ MATRIX / OUTPUT      │  │
│  └────────────────┘  │ │−4dB │ │  └──────────────────────┘  │
│                      │ └─────┘ │                            │
└──────────────────────┴─────────┴────────────────────────────┘
```

- **Left column** (signal in / tone): INPUT → FILTERS → EQ, stacked top→bottom.
- **Center bridge** (96px wide, full body height): the two LED-ladder meters
  (SATURATING, COMPRESSING) side by side, with the numeric GR readout below.
- **Right column** (dynamics / out): COMPRESSOR → DRIVE → MATRIX/OUTPUT, stacked.
- **Header** (36px): `HOLDOVER` wordmark (with `OVER` in accent), left-aligned. No
  tagline, no functional controls — purely an alignment anchor.

Both columns flex to equal height so their bottoms align; panels with fewer controls
(INPUT, DRIVE) keep the extra space as breathing room rather than enlarging knobs.

### Base size & resizing
- **Base size: 960 × 640 px (3:2).** This is the design size and the default open size.
- Aspect ratio locked to 3:2 via the editor constrainer; window scales uniformly
  (single `AffineTransform::scale`), exactly as the current editor already does.
- **Resize limits:** min 0.7× (672 × 448), max 1.6× (1536 × 1024).
- **Default/persisted size:** a fresh instance opens at 960 × 640. Persisted UI size
  continues to load, but the stored default is updated to the new base so existing
  "too small" defaults no longer apply.
- **Initial-size ordering:** in the editor constructor, read the persisted size and
  call `setSize()` *before* the first layout pass, so the UI lays out once and opens
  at full size rather than laying out at a stale default and then resizing.
- **Resize write cost:** `resized()` fires continuously during a drag and writes the
  current size back to the processor (`uiWidth`/`uiHeight`). Keep that write a plain
  cheap assignment — no allocation, no notifications — so dragging stays smooth.

---

## 3. Panel → control mapping (unchanged controls)

| Panel | Controls |
|---|---|
| **INPUT** | Gain L, Gain R, Input (3 knobs) |
| **FILTERS** | `FILTER IN` switch · HPF, HPF Peak, LPF, LPF Peak (4 knobs) |
| **EQ** | `EQ IN` switch · Bass (knob), Bass Hz (choice), Presence (knob), Pres Hz (knob), Treble (knob), Treble Hz (choice) |
| **COMPRESSOR** | `COMP IN` switch · `RMS`, `SC FILTER` LED buttons · Threshold, Behavior, Makeup (knobs) · Attack, Release, SC Src (choices) |
| **DRIVE** | Drive (knob) · MAS (choice) · `SAT`, `HEX`, `CURVE` LED buttons |
| **MATRIX / OUTPUT** | Dry/EQ, Comp, Sat, Output (4 knobs) · Dry Src (choice) · 3× `MUTE` + `CEILING` LED buttons |
| **METERS (bridge)** | SATURATING ladder, COMPRESSING ladder, GR numeric readout |

Knob rows are left-aligned with fixed-width cells, so the first three knob columns line
up across INPUT / FILTERS / EQ (and likewise on the right). EQ and COMPRESSOR each
resolve to a tidy six-cell row.

---

## 4. Components

All components share the existing palette: bg `#141416`, panel `#1c1c20`, accent
`#5ad1c4`, text `#d8d8dc`, hairline `#26262c`/`#2a2a30`.

- **Knob cell** — fixed 62px-wide cell: label (≈9px) above, **56px** rotary, value
  readout below. The rotary keeps the current true-circle look-and-feel (border ring +
  accent value arc + pointer), just larger. Identical everywhere.
- **Choice cell** — same 62px footprint: label above, a bordered dropdown box vertically
  positioned to align with the knob row.
- **Pill switch** (engage/IN) — a sliding 38×20 track with a 16px thumb plus a bold
  label; track turns accent and thumb slides right when on. Used for FILTER IN, EQ IN,
  COMP IN.
- **LED button** (mode) — bordered button with a label and a small indicator dot that
  glows accent when on. Used for RMS, SC FILTER, SAT, HEX, CURVE, CEILING. MUTE uses a
  red-glow variant (red dot + red border when active).

These are new look-and-feel/component treatments for `juce::ToggleButton`; the
underlying APVTS attachments are unchanged.

---

## 5. Affected files

- `Source/PluginEditor.{h,cpp}` — new base size (960×640), region layout (left col /
  bridge / right col), resize limits, default-size handling.
- `Source/ui/HoldoverLookAndFeel.{h,cpp}` — larger rotary; new `drawToggleButton`
  paths for pill-switch vs LED-button styles (distinguished by a button property/flag).
- `Source/ui/ControlHelpers.h` — knob/choice cell sizing; a helper to tag a toggle as
  pill-switch vs LED-button vs mute.
- `Source/ui/panels/Panel.h` — panel chrome (title underline, padding, column helper).
- `Source/ui/panels/*.h` — per-panel `resized()` updated to the new cell grid; mark
  engage toggles as switches, mode toggles as LED buttons.
- `Source/ui/LedMeter.{h,cpp}` — render the bridge ladder at the new size; vertical
  caption.

No changes to `PluginProcessor`, `Parameters`, DSP, or preset code, except the editor's
default UI-size constants.

---

## 6. Testing / verification

- Existing unit + pluginval tests must continue to pass unchanged (UI-only change).
- Manual verification: build, open in a host, confirm (a) opens at 960×640, (b) drags
  resize uniformly with no stretching, (c) switches and LED buttons toggle their
  bound parameters, (d) meters animate. Capture a screenshot for sign-off.

---

## 7. Open items

- Exact value-readout formatting per parameter (reuse current `Slider` text where it
  already reads well).
