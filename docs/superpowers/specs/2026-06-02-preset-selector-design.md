# In-UI Preset Selector — Design

**Date:** 2026-06-02
**Status:** Approved

## Goal

Let users browse and load presets directly in the plugin editor, and save the
current knob state as a reusable user preset — without going through the host's
program menu. Match the established pattern used in `plugin-treibend` and
`plugin-denneverb` rather than inventing a new one.

## Scope

- Factory preset selection (the existing 9 in `holdover::presets::all()`).
- User preset save + load, persisted to disk in a shared folder.
- Combined factory+user selector with prev/next stepping.
- A single `SAVE` action (save-as, name-prompted; same name overwrites).

Out of scope for v1: delete, rename, preset categories/folders, import/export.

## Reference implementation

`plugin-treibend/src/PresetManager.{h,cpp}` and its editor wiring are the
template. Differences from Treibend are called out explicitly below; everything
else mirrors it.

## Storage

- **Directory:** `File::getSpecialLocation(userApplicationDataDirectory)`
  `/ "Holdover" / "Presets"`, created on first use.
  On macOS this resolves to `~/Library/Application Support/Holdover/Presets`.
  (Same convention as both reference plugins — **not** `~/Documents`.)
- **File per preset:** `<sanitized-name>.preset`.
- **Format:** the whole APVTS state — `apvts.copyState().createXml()->writeTo(file, {})`.
  This is exactly how `HoldoverProcessor::getStateInformation` already serializes
  parameters, so user presets are just a parameter snapshot with no bespoke schema.
- **Name sanitization:** `juce::File::createLegalFileName(name)`; empty result is
  rejected (no write). This also prevents path-escape via `/` or `..`.

## Components

### 1. `PresetManager` — `Source/presets/PresetManager.{h,cpp}` (new)

Non-UI. Owns the combined factory+user list and all disk I/O. Namespace
`holdover`. Constructed with the APVTS (default dir) — `PresetManager presetManager { apvts };`.

Public API (mirrors Treibend):

```cpp
juce::StringArray getAllNames() const;   // factory names, then user file names (sorted)
int  getNumFactory() const;              // == presets::all().size()
void loadByIndex(int index);             // combined index space
void loadByName(const juce::String&);
void saveUser(const juce::String& name); // writes <dir>/<name>.preset, then selects it
void next();                             // loadByIndex(currentIndex + 1)
void prev();                             // loadByIndex(currentIndex - 1)
int  getCurrentIndex() const;
juce::String getCurrentName() const;
bool isModified() const;
void setModified(bool);
static juce::File defaultDir();
```

Behavior:

- **Factory load** (`index < getNumFactory()`): reuse the existing
  `holdover::presets::apply(apvts, index)` — no duplication of the factory bank.
- **User load** (`index >= getNumFactory()`): read `<name>.preset`,
  `apvts.replaceState(ValueTree::fromXml(...))`, **then run the same force-apply
  loop** `HoldoverProcessor::setStateInformation` uses — iterate the restored
  child params and `param->setValueNotifyingHost(param->convertTo0to1(stored))`.
  This is the one deliberate divergence from Treibend (which uses bare
  `replaceState`): Holdover documents that `replaceState` skips snapped
  bool/choice params whose raw value didn't change, so the loop is required for
  correct recall of toggles like `compMute`, `satEngage`, etc.
- Every successful load and save sets `modified = false` and updates `currentIndex`.
- `loadByIndex` is index-clamped and defensive against an empty list.

### 2. Processor additions — `PluginProcessor.{h,cpp}`

- Add member `PresetManager presetManager { apvts };` declared **after** `apvts`
  (construction-order dependency, same as Treibend).
- Add `PresetManager& getPresetManager() { return presetManager; }`.
- Add `juce::String currentPresetName;` — persisted, set by the editor on
  load/save, so the displayed selection survives reopen. Serialized in
  `getStateInformation` (a property on a small child tree) and restored in
  `setStateInformation`, mirroring Treibend's `currentPresetName` handling.
  **Defaults to `presetManager.getCurrentName()` (i.e. `"Init / Flat"`, the
  factory entry at index 0) on first instantiation** so the combo never shows
  blank before any load fires.
- **Host program interface delegates to `PresetManager`** so there is a single
  source of truth for the current selection (the editor combo syncs to
  `getCurrentIndex()`, so the processor must not keep a separate stale index):
  - `getNumPrograms()` → `getNumFactory()` (host menu stays factory-only and
    count is stable — no dynamic program count, avoiding the fragility flagged
    when this was scoped).
  - `setCurrentProgram(i)` → `presetManager.loadByIndex(i)` (i is within the
    factory range from the host).
  - `getCurrentProgram()` → `jlimit(0, getNumFactory()-1, presetManager.getCurrentIndex())`
    (a user preset reports as the clamped factory index; the host menu is
    factory-only, so this is acceptable).
  - `getProgramName(i)` unchanged (`presets::all()[i].name`).
  - The `currentProgram_` member is removed — `PresetManager` holds the index.
  This way a host-driven factory program change updates `PresetManager`, and the
  editor combo reflects it on the next timer tick.

### 3. `PresetBar` — `Source/ui/PresetBar.{h,cpp}` (new)

A small `juce::Component` holding the selector controls, placed in the header.
Owns: `juce::ComboBox presetBox`, `juce::TextButton prevBtn` (`‹`),
`nextBtn` (`›`), `saveBtn` (`SAVE`). Constructed with a `HoldoverProcessor&`.

- `refreshList()` — clear `presetBox`, add `getAllNames()` (IDs `i+1`), select
  `getCurrentIndex()` with `dontSendNotification`.
- `presetBox.onChange` — on a valid selection, `getPresetManager().loadByIndex(idx)`
  and update `processor.currentPresetName = getCurrentName()`.
- `prevBtn` / `nextBtn` — call `prev()` / `next()`, then `refreshList()`.
- `saveBtn` — open a `juce::AlertWindow` ("Save Preset", text editor pre-filled
  with the current preset name), `Save`/`Cancel`. On Save with a non-empty
  trimmed name: `getPresetManager().saveUser(name)`,
  `processor.currentPresetName = name`, `refreshList()`. Guard the modal callback
  with `juce::Component::SafePointer` against editor destruction (Treibend's
  exact guard).
- **Pre-filling the current name is what makes one button cover both cases:**
  keep the name → overwrite; change it → new preset. No second control.
- `syncSelection()` — if `presetBox.getSelectedItemIndex() != getCurrentIndex()`,
  re-select without notification. Called from the editor timer.

Section headings (`addSectionHeading("FACTORY")` / `"USER"`) are a nice-to-have;
include if it's a one-liner against the combined list, skip if it complicates the
index mapping.

### 4. Editor wiring — `PluginEditor.{h,cpp}`

- `Content` gains `PresetBar presetBar_;`, constructed with the processor (so it
  needs the `HoldoverProcessor&`, not just the APVTS — widen `Content`'s ctor).
- `Content::resized()` carves the combo strip out of the **right** of
  `headerArea_`: the `HOLD OVER` wordmark keeps the left; the bar gets the
  remainder, laid out right-to-left `SAVE | › | ‹ | combo` (Treibend's order).
- The existing 30 Hz `timerCallback` already forwards to `Content`; add a
  `presetBar_.syncSelection()` call on each tick (cheap; mirrors Treibend's
  8 Hz sync — 30 Hz is fine).

### 5. Modified indicator (required)

`PresetBar` registers as an `APVTS::Listener`; the first parameter change after a
load sets `getPresetManager().setModified(true)`. When modified, show a trailing
` *` on the combo's displayed text (or a small dot). Cleared on load/save. This
gives the user honest feedback that they have unsaved tweaks — without it, an
edited preset reads as if it still matches what's on disk.

## CMake

- Add `presets/PresetManager.cpp` and `ui/PresetBar.cpp` to `Source/CMakeLists.txt`
  (alongside `presets/FactoryPresets.cpp`).
- Add `tests/test_presetmanager.cpp` to `tests/CMakeLists.txt`.

## Error handling

- Save failure (illegal/empty name, unwritable dir): no file written; surface a
  brief `AlertWindow` message. Never crash, never partially write.
- Corrupt/missing `.preset` on load: parse fails → ignore, leave current state
  untouched (optionally log). No exception escapes.
- Empty preset folder: only factory entries appear; everything still works.

## Testing — `tests/test_presetmanager.cpp` (new)

Construct `PresetManager` against a **temp directory** (use the two-arg ctor with
an explicit dir, like Treibend) so tests never touch the real user folder.

- Factory: `getNumFactory()` == `presets::all().size()`; `getAllNames()` starts
  with the factory names in order.
- Save → file exists at `<dir>/<name>.preset`; `getAllNames()` now includes it;
  `getCurrentName()` == the saved name; `isModified()` == false.
- Round-trip: set a distinctive param mix (including a **snapped bool/choice**,
  e.g. `compMute` and `masMode`), save, load a different preset, reload the saved
  one → all params (bools/choices included) match. This is the regression guard
  for the force-apply divergence.
- Overwrite: save name "X", change params, save "X" again → still one file,
  contents reflect the second save.
- Sanitization: a name with `/` or `..` writes inside the dir (legalized
  filename), never escapes it; empty/whitespace name writes nothing.
- `next`/`prev` wrap/clamp consistently with `getCurrentIndex()`.

UI (`PresetBar`, editor layout) stays untested, consistent with the rest of the
suite.

## Out of scope / YAGNI

Delete, rename, preset folders/tags, drag-reorder, factory-preset editing,
cloud/sharing, MIDI program-change mapping beyond the existing host interface.
