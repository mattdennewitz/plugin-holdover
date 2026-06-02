# Holdover Console UI Rebuild — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild the Holdover editor into a strictly-aligned two-column console layout (tone-shaping left, dynamics/output right, meters in a center bridge) with bigger uniform knobs, obvious switch/LED-button affordances, and a 1020×680 (3:2) window that opens full-size and resizes proportionally.

**Architecture:** UI-only change. A shared `Source/ui/Layout.h` defines layout constants and a fixed-width cell helper so every knob is identical and columns align. The look-and-feel grows two new toggle renderers (pill switch, LED button) selected via a per-button property. Panels keep their existing controls and APVTS attachments; only their `resized()` and toggle styling change. The editor lays content out once at the base size and scales uniformly with a locked aspect ratio (existing mechanism).

**Tech Stack:** JUCE 8.0.4, C++20, CMake. DSP and parameters are untouched.

**Verification approach (read this first):** The test target (`HoldoverTests`) links only `HoldoverDSP` — it has no GUI and cannot assert pixel layout. There is no automated seam for rendering/layout here, so forcing red-green unit tests on this work would be low-value. Instead, every task ends with: (1) an **incremental build** that must succeed, and (2) **`ctest`** to prove the DSP suite stays green (regression guard — these tests must never change). The final task is a structured **manual visual verification** in a host plus a clean reinstall. Do not add GUI test targets; that is out of scope (YAGNI).

**One-time setup (run once before Task 1):**

```bash
cd /Users/matt/src/plugin-underwhelmer
# Confirm a clean baseline build + green tests before changing anything.
cmake --build build --target HoldoverTests -j && ctest --test-dir build --output-on-failure
```
Expected: build succeeds, all tests pass. If not, stop and fix the baseline first.

---

## Task 1: Base size, default open size, and resize behavior

Sets the window to 1020×680, makes a fresh instance open at full size, fixes initial-size ordering, and sets 0.7×–1.6× resize limits.

**Files:**
- Modify: `Source/PluginEditor.h:25-26` (base size constants)
- Modify: `Source/PluginProcessor.h:39` (default UI size)
- Modify: `Source/PluginEditor.cpp:58-69` (constructor: ordering + resize limits)

- [ ] **Step 1: Update base size constants**

In `Source/PluginEditor.h`, change:

```cpp
    static constexpr int kBaseWidth  = 900;
    static constexpr int kBaseHeight = 700;
```
to:
```cpp
    static constexpr int kBaseWidth  = 1020;
    static constexpr int kBaseHeight = 680;
```

- [ ] **Step 2: Update the processor's default UI size**

In `Source/PluginProcessor.h`, change:

```cpp
    int uiWidth = 900, uiHeight = 700;
```
to:
```cpp
    int uiWidth = 1020, uiHeight = 680;
```

- [ ] **Step 3: Fix constructor ordering and resize limits**

In `Source/PluginEditor.cpp`, replace the constructor body (lines 58-69) with:

```cpp
HoldoverEditor::HoldoverEditor(HoldoverProcessor& p)
    : AudioProcessorEditor(&p), processor(p), content_(p.apvts) {
    setLookAndFeel(&lnf_);
    addAndMakeVisible(content_);

    setResizable(true, true);
    // Lock to the base aspect ratio so uniform scaling never distorts.
    getConstrainer()->setFixedAspectRatio((double) kBaseWidth / (double) kBaseHeight);
    setResizeLimits((int) std::round(kBaseWidth * 0.7), (int) std::round(kBaseHeight * 0.7),
                    (int) std::round(kBaseWidth * 1.6), (int) std::round(kBaseHeight * 1.6));

    // Read the persisted size and size the window ONCE, before any other layout work,
    // so a fresh instance opens at full base size rather than a stale default.
    setSize(processor.uiWidth, processor.uiHeight);
    startTimerHz(30);
}
```

- [ ] **Step 4: Build**

Run: `cmake --build build --target Holdover -j`
Expected: builds successfully (links and copies the plugin).

- [ ] **Step 5: Regression guard — run the DSP tests**

Run: `ctest --test-dir build --output-on-failure`
Expected: all tests PASS (no DSP code changed).

- [ ] **Step 6: Commit**

```bash
git add Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp
git commit -m "feat(ui): 1020x680 base, open full-size, 0.7x-1.6x resize limits"
```

---

## Task 2: Shared layout constants and cell helper

Creates one source of truth for cell sizing so knobs are uniform and rows align everywhere.

**Files:**
- Create: `Source/ui/Layout.h`

- [ ] **Step 1: Create `Source/ui/Layout.h`**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace holdover::ui {

// One source of truth for editor layout. Every knob/choice cell is identical, so
// controls line up vertically across panels and both columns align.
constexpr int kCellW        = 62;   // width of one knob/choice cell
constexpr int kCellGap      = 8;    // horizontal gap between cells in a row
constexpr int kLabelH       = 14;   // label strip above a control
constexpr int kKnobH        = 56;   // rotary diameter target
constexpr int kValueH       = 16;   // value readout strip below a knob
constexpr int kCellH        = kLabelH + kKnobH + kValueH; // full knob-cell height (86)
constexpr int kChoiceBoxH   = 22;   // dropdown box height
constexpr int kToggleH      = 24;   // switch / LED-button row height
constexpr int kRowGap       = 10;   // vertical gap between rows / panels / regions
constexpr int kPanelPadX    = 12;   // panel horizontal padding
constexpr int kPanelPadTop  = 8;    // gap below a panel title before content
constexpr int kPanelPadBot  = 10;   // panel bottom padding
constexpr int kPanelTitleH  = 22;   // panel title bar height
constexpr int kHeaderH      = 36;   // top wordmark band
constexpr int kBridgeW      = 96;   // meter bridge width

// Lay out `n` fixed-width (kCellW) cells left-to-right in `area`, separated by
// kCellGap. Fixed width (not flex) is what makes a knob in INPUT identical to one in
// EQ, and makes their columns line up vertically.
inline std::vector<juce::Rectangle<int>> cells(juce::Rectangle<int> area, int n) {
    std::vector<juce::Rectangle<int>> out;
    out.reserve((size_t) n);
    for (int i = 0; i < n; ++i) {
        out.push_back(area.removeFromLeft(kCellW));
        if (i < n - 1) area.removeFromLeft(kCellGap);
    }
    return out;
}

} // namespace holdover::ui
```

- [ ] **Step 2: Build (header-only; compile a dependent TU to catch errors)**

Run: `cmake --build build --target Holdover -j`
Expected: builds successfully (nothing includes it yet, so this just confirms it parses once included later — it will be exercised in Task 4+).

- [ ] **Step 3: Commit**

```bash
git add Source/ui/Layout.h
git commit -m "feat(ui): add shared layout constants and fixed-cell helper"
```

---

## Task 3: Control helpers — uniform cell layout and toggle styles

Resizes knob/choice cells to the shared constants and adds a toggle-style tag (switch / LED button / mute) carried on the button so the look-and-feel can render it.

**Files:**
- Modify: `Source/ui/ControlHelpers.h`

- [ ] **Step 1: Rewrite `Source/ui/ControlHelpers.h`**

Replace the entire file with:

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Layout.h"

namespace holdover {

using APVTS = juce::AudioProcessorValueTreeState;

// How a toggle is drawn. Carried on the button via a property so the look-and-feel
// can pick the renderer without subclassing ToggleButton.
enum class ToggleStyle { Switch, Led, Mute };

inline void tagToggleStyle(juce::ToggleButton& b, ToggleStyle s) {
    b.getProperties().set("hldStyle", (int) s);
}

// A labelled rotary knob bound to an APVTS parameter. Owns its attachment.
struct AttachedKnob {
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag,
                          juce::Slider::TextBoxBelow };
    juce::Label  label;
    std::unique_ptr<APVTS::SliderAttachment> attachment;

    void attach(juce::Component& parent, APVTS& s, const juce::String& id, const juce::String& text) {
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, ui::kCellW - 8, ui::kValueH);
        parent.addAndMakeVisible(slider);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(10.0f)));
        parent.addAndMakeVisible(label);
        attachment = std::make_unique<APVTS::SliderAttachment>(s, id, slider);
    }
    // Label on top (kLabelH); the slider fills the rest and draws the rotary above its
    // own TextBoxBelow, so the rotary lands at ~kKnobH across every panel.
    void layout(juce::Rectangle<int> area) {
        label.setBounds(area.removeFromTop(ui::kLabelH));
        slider.setBounds(area);
    }
};

struct AttachedToggle {
    juce::ToggleButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
    void attach(juce::Component& parent, APVTS& s, const juce::String& id,
                const juce::String& text, ToggleStyle style) {
        button.setButtonText(text);
        tagToggleStyle(button, style);
        parent.addAndMakeVisible(button);
        attachment = std::make_unique<APVTS::ButtonAttachment>(s, id, button);
    }
    void layout(juce::Rectangle<int> area) { button.setBounds(area); }
};

struct AttachedChoice {
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
    void attach(juce::Component& parent, APVTS& s, const juce::String& id,
                const juce::String& text, const juce::StringArray& items) {
        box.addItemList(items, 1);
        parent.addAndMakeVisible(box);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(10.0f)));
        parent.addAndMakeVisible(label);
        attachment = std::make_unique<APVTS::ComboBoxAttachment>(s, id, box);
    }
    // Label on top (kLabelH); the dropdown box is vertically centred in the remaining
    // space so it lines up with the rotaries in the same row.
    void layout(juce::Rectangle<int> area) {
        label.setBounds(area.removeFromTop(ui::kLabelH));
        box.setBounds(area.withSizeKeepingCentre(area.getWidth(), ui::kChoiceBoxH));
    }
};

} // namespace holdover
```

- [ ] **Step 2: Build**

Run: `cmake --build build --target Holdover -j`
Expected: FAILS to link/compile in the panels, because every `AttachedToggle::attach(...)` call site still passes the old 4-argument signature (no `ToggleStyle`). That is expected — Tasks 5 and 6 update the call sites. To verify just this file parses, instead build only the object that includes it indirectly is not possible in isolation; proceed to Step 3 and accept that the tree is red until Task 6.

> NOTE TO IMPLEMENTER: Tasks 3→6 are a single logical change split for review. The build is intentionally red after Task 3 until the panels are updated (Task 5, 6). Do not "fix" the panels early; follow the tasks. Run the green build at the end of Task 6.

- [ ] **Step 3: Commit**

```bash
git add Source/ui/ControlHelpers.h
git commit -m "feat(ui): uniform knob/choice cells; tag toggles with a style"
```

---

## Task 4: Look-and-feel — bigger rotary, pill switch, LED button, mute

Renders the two new toggle styles and keeps the true-circle rotary (it already scales with the slot, so bigger cells give bigger knobs automatically).

**Files:**
- Modify: `Source/ui/HoldoverLookAndFeel.cpp:46-56` (`drawToggleButton`)
- Modify: `Source/ui/HoldoverLookAndFeel.h` (add color constant)

- [ ] **Step 1: Add a mute/red constant**

In `Source/ui/HoldoverLookAndFeel.h`, inside the class after `kText`, add:

```cpp
    static constexpr juce::uint32 kMute   = 0xffd65b5b;
    static constexpr juce::uint32 kOffDot = 0xff3a3a40;
    static constexpr juce::uint32 kTrack  = 0xff26262c;
```

- [ ] **Step 2: Replace `drawToggleButton` in `Source/ui/HoldoverLookAndFeel.cpp`**

Replace the existing `drawToggleButton` (lines 46-56) with:

```cpp
void HoldoverLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b,
                                           bool highlighted, bool) {
    const bool on = b.getToggleState();
    const auto style = (ToggleStyle) (int) b.getProperties().getWithDefault("hldStyle", (int) ToggleStyle::Led);
    auto bounds = b.getLocalBounds().toFloat().reduced(1.0f);

    if (style == ToggleStyle::Switch) {
        // Sliding pill on the left, bold label to its right.
        const float trackW = 38.0f, trackH = 20.0f;
        auto track = juce::Rectangle<float>(trackW, trackH)
                         .withY(bounds.getCentreY() - trackH * 0.5f).withX(bounds.getX());
        g.setColour(juce::Colour(on ? kAccent : kTrack));
        g.fillRoundedRectangle(track, trackH * 0.5f);
        const float thumbD = trackH - 4.0f;
        const float thumbX = on ? track.getRight() - thumbD - 2.0f : track.getX() + 2.0f;
        g.setColour(juce::Colour(on ? 0xff10100f : 0xff6a6a72));
        g.fillEllipse(thumbX, track.getY() + 2.0f, thumbD, thumbD);
        g.setColour(juce::Colour(on ? kText : 0xff9a9aa2));
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(b.getButtonText(),
                   bounds.withTrimmedLeft(trackW + 8.0f), juce::Justification::centredLeft);
        return;
    }

    // LED button (and mute variant): bordered button, label, glowing indicator dot.
    const juce::uint32 accent = (style == ToggleStyle::Mute) ? kMute : kAccent;
    g.setColour(juce::Colour(on ? (style == ToggleStyle::Mute ? 0xff221416u : 0xff15201fu) : 0xff1a1a1eu));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(on ? accent : 0xff2a2a30));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    if (highlighted) { g.setColour(juce::Colour(0x18ffffff)); g.fillRoundedRectangle(bounds, 4.0f); }

    const float dotD = 7.0f;
    auto dot = juce::Rectangle<float>(dotD, dotD)
                   .withY(bounds.getCentreY() - dotD * 0.5f)
                   .withX(bounds.getRight() - dotD - 8.0f);
    if (on) { g.setColour(juce::Colour(accent).withAlpha(0.35f)); g.fillEllipse(dot.expanded(2.5f)); }
    g.setColour(juce::Colour(on ? accent : kOffDot));
    g.fillEllipse(dot);

    g.setColour(juce::Colour(on ? kText : 0xff9a9aa2));
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.drawText(b.getButtonText(),
               b.getLocalBounds().withTrimmedRight(18).withTrimmedLeft(8),
               juce::Justification::centredLeft);
}
```

- [ ] **Step 3: Ensure the style enum is visible to the look-and-feel**

At the top of `Source/ui/HoldoverLookAndFeel.cpp`, after the existing include, add:

```cpp
#include "ControlHelpers.h"
```
(`ControlHelpers.h` defines `ToggleStyle`. This is header-only and pulls no DSP.)

- [ ] **Step 4: Commit** (tree still intentionally red until Task 6)

```bash
git add Source/ui/HoldoverLookAndFeel.h Source/ui/HoldoverLookAndFeel.cpp
git commit -m "feat(ui): render pill-switch and LED-button toggle styles"
```

---

## Task 5: Panel chrome and left-column panels (Input, Filters, EQ)

Gives panels an underlined title and lays the three left panels on the uniform grid, tagging engage toggles as switches.

**Files:**
- Modify: `Source/ui/panels/Panel.h`
- Modify: `Source/ui/panels/InputPanel.h`
- Modify: `Source/ui/panels/FilterPanel.h`
- Modify: `Source/ui/panels/EqPanel.h`

- [ ] **Step 1: Rewrite `Source/ui/panels/Panel.h`**

Replace the entire file with:

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../HoldoverLookAndFeel.h"
#include "../Layout.h"

namespace holdover {

class Panel : public juce::Component {
public:
    explicit Panel(juce::String title) : title_(std::move(title)) {}
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour(HoldoverLookAndFeel::kPanel));
        g.fillRoundedRectangle(b, 6.0f);

        auto titleRow = getLocalBounds().removeFromTop(ui::kPanelTitleH).reduced(ui::kPanelPadX, 0);
        g.setColour(juce::Colour(HoldoverLookAndFeel::kText));
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(title_, titleRow, juce::Justification::centredLeft);

        // Hairline under the title.
        g.setColour(juce::Colour(0xff26262c));
        g.fillRect(ui::kPanelPadX, ui::kPanelTitleH - 1,
                   getWidth() - 2 * ui::kPanelPadX, 1);
    }
    // Inner content rectangle: below the title, padded.
    juce::Rectangle<int> contentArea() const {
        auto a = getLocalBounds().reduced(ui::kPanelPadX, 0);
        a.removeFromTop(ui::kPanelTitleH + ui::kPanelPadTop);
        a.removeFromBottom(ui::kPanelPadBot);
        return a;
    }
private:
    juce::String title_;
};

} // namespace holdover
```

> NOTE: the old `Panel::columns(...)` static helper is removed. No panel uses it after this rebuild (they use `ui::cells`). If any reference remains, it is in a panel being rewritten in this task or Task 6.

- [ ] **Step 2: Rewrite `Source/ui/panels/InputPanel.h`**

```cpp
#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"

namespace holdover {

class InputPanel : public Panel {
public:
    explicit InputPanel(APVTS& s) : Panel("INPUT") {
        gainL_.attach(*this, s, "gainL", "Gain L");
        gainR_.attach(*this, s, "gainR", "Gain R");
        input_.attach(*this, s, "input", "Input");
    }
    void resized() override {
        auto a = contentArea();
        auto row = a.removeFromTop(ui::kCellH);
        auto c = ui::cells(row, 3);
        gainL_.layout(c[0]); gainR_.layout(c[1]); input_.layout(c[2]);
    }
private:
    AttachedKnob gainL_, gainR_, input_;
};

} // namespace holdover
```

- [ ] **Step 3: Rewrite `Source/ui/panels/FilterPanel.h`**

```cpp
#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"

namespace holdover {

class FilterPanel : public Panel {
public:
    explicit FilterPanel(APVTS& s) : Panel("FILTERS") {
        engage_.attach(*this, s, "filtEngage", "FILTER IN", ToggleStyle::Switch);
        hpfFreq_.attach(*this, s, "hpfFreq", "HPF");
        hpfPeak_.attach(*this, s, "hpfPeak", "HPF Peak");
        lpfFreq_.attach(*this, s, "lpfFreq", "LPF");
        lpfPeak_.attach(*this, s, "lpfPeak", "LPF Peak");
    }
    void resized() override {
        auto a = contentArea();
        engage_.layout(a.removeFromTop(ui::kToggleH).removeFromLeft(130));
        a.removeFromTop(ui::kRowGap);
        auto c = ui::cells(a.removeFromTop(ui::kCellH), 4);
        hpfFreq_.layout(c[0]); hpfPeak_.layout(c[1]); lpfFreq_.layout(c[2]); lpfPeak_.layout(c[3]);
    }
private:
    AttachedToggle engage_;
    AttachedKnob hpfFreq_, hpfPeak_, lpfFreq_, lpfPeak_;
};

} // namespace holdover
```

- [ ] **Step 4: Rewrite `Source/ui/panels/EqPanel.h`**

```cpp
#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class EqPanel : public Panel {
public:
    explicit EqPanel(APVTS& s) : Panel("EQ") {
        engage_.attach(*this, s, "eqEngage", "EQ IN", ToggleStyle::Switch);
        bassDb_.attach(*this, s, "bassDb", "Bass");
        bassFreq_.attach(*this, s, "bassFreq", "Bass Hz", params::bassFreqOptions);
        presDb_.attach(*this, s, "presenceDb", "Presence");
        presFreq_.attach(*this, s, "presenceFreq", "Pres Hz");
        trebDb_.attach(*this, s, "trebleDb", "Treble");
        trebFreq_.attach(*this, s, "trebleFreq", "Treble Hz", params::trebleFreqOptions);
    }
    void resized() override {
        auto a = contentArea();
        engage_.layout(a.removeFromTop(ui::kToggleH).removeFromLeft(110));
        a.removeFromTop(ui::kRowGap);
        auto c = ui::cells(a.removeFromTop(ui::kCellH), 6);
        bassDb_.layout(c[0]); bassFreq_.layout(c[1]);
        presDb_.layout(c[2]); presFreq_.layout(c[3]);
        trebDb_.layout(c[4]); trebFreq_.layout(c[5]);
    }
private:
    AttachedToggle engage_;
    AttachedKnob bassDb_, presDb_, presFreq_, trebDb_;
    AttachedChoice bassFreq_, trebFreq_;
};

} // namespace holdover
```

- [ ] **Step 5: Commit** (tree still red until Task 6 finishes the right panels)

```bash
git add Source/ui/panels/Panel.h Source/ui/panels/InputPanel.h Source/ui/panels/FilterPanel.h Source/ui/panels/EqPanel.h
git commit -m "feat(ui): underlined panel chrome; grid-align Input/Filters/EQ"
```

---

## Task 6: Right-column panels (Compressor, Drive, Matrix/Output)

Lays the three right panels on the uniform grid and tags mode toggles as LED buttons (mutes as red). After this task the tree builds green again.

**Files:**
- Modify: `Source/ui/panels/CompressorPanel.h`
- Modify: `Source/ui/panels/DrivePanel.h`
- Modify: `Source/ui/panels/MatrixPanel.h`

- [ ] **Step 1: Rewrite `Source/ui/panels/CompressorPanel.h`**

```cpp
#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class CompressorPanel : public Panel {
public:
    explicit CompressorPanel(APVTS& s) : Panel("COMPRESSOR") {
        engage_.attach(*this, s, "compEngage", "COMP IN", ToggleStyle::Switch);
        rms_.attach(*this, s, "rmsMode", "RMS", ToggleStyle::Led);
        scFilt_.attach(*this, s, "scFilter", "SC FILTER", ToggleStyle::Led);
        thr_.attach(*this, s, "threshold", "Threshold");
        beh_.attach(*this, s, "behavior", "Behavior");
        mak_.attach(*this, s, "makeup", "Makeup");
        atk_.attach(*this, s, "attack", "Attack", params::timeOptions);
        rel_.attach(*this, s, "release", "Release", params::timeOptions);
        sc_.attach(*this, s, "scSource", "SC Src", params::scSourceOptions);
    }
    void resized() override {
        auto a = contentArea();
        auto tog = a.removeFromTop(ui::kToggleH);
        engage_.layout(tog.removeFromLeft(120)); tog.removeFromLeft(ui::kCellGap);
        rms_.layout(tog.removeFromLeft(70));     tog.removeFromLeft(ui::kCellGap);
        scFilt_.layout(tog.removeFromLeft(108));
        a.removeFromTop(ui::kRowGap);
        auto c = ui::cells(a.removeFromTop(ui::kCellH), 6);
        thr_.layout(c[0]); beh_.layout(c[1]); mak_.layout(c[2]);
        atk_.layout(c[3]); rel_.layout(c[4]); sc_.layout(c[5]);
    }
private:
    AttachedToggle engage_, rms_, scFilt_;
    AttachedKnob thr_, beh_, mak_;
    AttachedChoice atk_, rel_, sc_;
};

} // namespace holdover
```

- [ ] **Step 2: Rewrite `Source/ui/panels/DrivePanel.h`**

```cpp
#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class DrivePanel : public Panel {
public:
    explicit DrivePanel(APVTS& s) : Panel("DRIVE") {
        drive_.attach(*this, s, "drive", "Drive");
        mas_.attach(*this, s, "masMode", "MAS", params::masOptions);
        sat_.attach(*this, s, "satEngage", "SAT", ToggleStyle::Led);
        hex_.attach(*this, s, "hexEngage", "HEX", ToggleStyle::Led);
        curve_.attach(*this, s, "curve", "CURVE", ToggleStyle::Led);
    }
    void resized() override {
        auto a = contentArea();
        auto row = a.removeFromTop(ui::kCellH);
        auto driveCell = row.removeFromLeft(ui::kCellW); row.removeFromLeft(ui::kCellGap);
        auto masCell   = row.removeFromLeft(ui::kCellW); row.removeFromLeft(ui::kCellGap * 2);
        drive_.layout(driveCell);
        mas_.layout(masCell);
        // Three LED buttons stacked, vertically centred in the remaining width.
        auto stack = row.withSizeKeepingCentre(juce::jmin(row.getWidth(), 120),
                                               3 * ui::kToggleH + 2 * 6);
        sat_.layout(stack.removeFromTop(ui::kToggleH));   stack.removeFromTop(6);
        hex_.layout(stack.removeFromTop(ui::kToggleH));   stack.removeFromTop(6);
        curve_.layout(stack.removeFromTop(ui::kToggleH));
    }
private:
    AttachedKnob drive_;
    AttachedChoice mas_;
    AttachedToggle sat_, hex_, curve_;
};

} // namespace holdover
```

- [ ] **Step 3: Rewrite `Source/ui/panels/MatrixPanel.h`**

```cpp
#pragma once
#include "Panel.h"
#include "../ControlHelpers.h"
#include "../../Parameters.h"

namespace holdover {

class MatrixPanel : public Panel {
public:
    explicit MatrixPanel(APVTS& s) : Panel("MATRIX / OUTPUT") {
        dryEq_.attach(*this, s, "dryEqFeedLevel", "Dry / EQ");
        comp_.attach(*this, s, "compFeedLevel", "Comp");
        sat_.attach(*this, s, "satFeedLevel", "Sat");
        dryEqSrc_.attach(*this, s, "dryEqSource", "Dry Src", params::dryEqSrcOptions);
        out_.attach(*this, s, "output", "Output");
        dryEqMute_.attach(*this, s, "dryEqMute", "MUTE", ToggleStyle::Mute);
        compMute_.attach(*this, s, "compMute", "MUTE", ToggleStyle::Mute);
        satMute_.attach(*this, s, "satMute", "MUTE", ToggleStyle::Mute);
        ceiling_.attach(*this, s, "ceiling", "CEILING", ToggleStyle::Led);
    }
    void resized() override {
        auto a = contentArea();
        auto c = ui::cells(a.removeFromTop(ui::kCellH), 5);
        dryEq_.layout(c[0]); comp_.layout(c[1]); sat_.layout(c[2]);
        dryEqSrc_.layout(c[3]); out_.layout(c[4]);
        a.removeFromTop(ui::kRowGap);
        auto tog = a.removeFromTop(ui::kToggleH);
        auto m = ui::cells(tog, 3); // mutes align under the first three feed knobs
        dryEqMute_.layout(m[0]); compMute_.layout(m[1]); satMute_.layout(m[2]);
        ceiling_.layout(tog.removeFromRight(100));
    }
private:
    AttachedKnob dryEq_, comp_, sat_, out_;
    AttachedToggle dryEqMute_, compMute_, satMute_, ceiling_;
    AttachedChoice dryEqSrc_;
};

} // namespace holdover
```

- [ ] **Step 4: Build (tree should be GREEN again)**

Run: `cmake --build build --target Holdover -j`
Expected: builds successfully — all `AttachedToggle::attach` call sites now pass a `ToggleStyle`.

- [ ] **Step 5: Regression guard — run the DSP tests**

Run: `ctest --test-dir build --output-on-failure`
Expected: all tests PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/ui/panels/CompressorPanel.h Source/ui/panels/DrivePanel.h Source/ui/panels/MatrixPanel.h
git commit -m "feat(ui): grid-align Compressor/Drive/Matrix; LED + mute toggles"
```

---

## Task 7: Editor content layout — header, two columns, meter bridge

Reworks the editor's `Content` into header + two equal columns (three equal-height panels each) + center meter bridge, and renders the meter ladders with vertical captions.

**Files:**
- Modify: `Source/ui/LedMeter.cpp` (vertical caption)
- Modify: `Source/PluginEditor.h` (Content paint override + stored regions)
- Modify: `Source/PluginEditor.cpp` (`Content::resized`, `Content::paint`)

- [ ] **Step 1: Vertical caption in `Source/ui/LedMeter.cpp`**

Replace the entire file with:

```cpp
#include "LedMeter.h"

namespace holdover {

void LedMeter::paint(juce::Graphics& g) {
    auto area = getLocalBounds();
    auto capArea = area.removeFromBottom(64); // strip for the rotated caption

    const int lit = (int) std::round(level_ * kSegments);
    const float segH = (float) area.getHeight() / kSegments;
    for (int i = 0; i < kSegments; ++i) {
        auto seg = juce::Rectangle<float>((float) area.getX(),
            area.getBottom() - (i + 1) * segH + 1.0f,
            (float) area.getWidth(), segH - 2.0f);
        const float frac = (float) i / (kSegments - 1);
        juce::Colour on = frac < 0.66f ? juce::Colour(0xff5ad17a)
                        : frac < 0.85f ? juce::Colour(0xffd1c45a)
                                       : juce::Colour(0xffd15a5a);
        g.setColour(i < lit ? on : juce::Colour(0xff26262c));
        g.fillRoundedRectangle(seg, 2.0f);
    }

    // Caption rotated 90° so it reads bottom-to-top beside the narrow ladder.
    juce::Graphics::ScopedSaveState save(g);
    const auto c = capArea.getCentre().toFloat();
    g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, c.x, c.y));
    auto txt = juce::Rectangle<float>(0.0f, 0.0f, (float) capArea.getHeight(), (float) capArea.getWidth())
                   .withCentre(c);
    g.setColour(juce::Colour(0xff9a9aa2));
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText(caption_, txt, juce::Justification::centred);
}

} // namespace holdover
```

- [ ] **Step 2: Add Content paint + region members in `Source/PluginEditor.h`**

In the `Content` class declaration, add a `paint` override and two stored rectangles. Replace the `Content` class (lines 33-50) with:

```cpp
    class Content : public juce::Component {
    public:
        explicit Content(juce::AudioProcessorValueTreeState&);
        void paint(juce::Graphics&) override;
        void resized() override;
        void updateMeters(float saturation, float gainReductionDb);

    private:
        InputPanel input_;
        FilterPanel filter_;
        EqPanel eq_;
        CompressorPanel comp_;
        DrivePanel drive_;
        MatrixPanel matrix_;

        LedMeter satMeter_ { "SATURATING" };
        LedMeter grMeter_  { "COMPRESSING" };
        juce::Label grReadout_;

        juce::Rectangle<int> headerArea_, bridgeArea_; // painted in paint()
    };
```

Add the include for `Layout.h` near the top of `Source/PluginEditor.h` (after the panel includes):

```cpp
#include "ui/Layout.h"
```

- [ ] **Step 3: Rewrite `Content::resized` and add `Content::paint` in `Source/PluginEditor.cpp`**

Replace `Content::resized` (lines 23-48) with the following two methods:

```cpp
void HoldoverEditor::Content::paint(juce::Graphics& g) {
    // Wordmark: "HOLD" in text, "OVER" in accent.
    juce::Font f(juce::FontOptions(19.0f, juce::Font::bold));
    g.setFont(f);
    juce::GlyphArrangement ga;
    ga.addLineOfText(f, "HOLD", 0.0f, 0.0f);
    const float holdW = ga.getBoundingBox(0, -1, true).getWidth();
    auto h = headerArea_;
    g.setColour(juce::Colour(HoldoverLookAndFeel::kText));
    g.drawText("HOLD", h, juce::Justification::centredLeft);
    g.setColour(juce::Colour(HoldoverLookAndFeel::kAccent));
    g.drawText("OVER", h.withTrimmedLeft((int) std::ceil(holdW)), juce::Justification::centredLeft);

    // Meter bridge background + title.
    g.setColour(juce::Colour(HoldoverLookAndFeel::kPanel));
    g.fillRoundedRectangle(bridgeArea_.toFloat(), 6.0f);
    g.setColour(juce::Colour(HoldoverLookAndFeel::kText));
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.drawText("METERS", bridgeArea_.withTrimmedTop(6).removeFromTop(ui::kPanelTitleH - 6),
               juce::Justification::centred);
}

void HoldoverEditor::Content::resized() {
    using namespace ui;
    auto area = getLocalBounds().reduced(12);

    headerArea_ = area.removeFromTop(kHeaderH).reduced(4, 0);
    area.removeFromTop(kRowGap);

    // Left column | bridge | right column.
    const int colW = (area.getWidth() - kBridgeW - 2 * kRowGap) / 2;
    auto leftCol  = area.removeFromLeft(colW);
    area.removeFromLeft(kRowGap);
    bridgeArea_   = area.removeFromLeft(kBridgeW);
    area.removeFromLeft(kRowGap);
    auto rightCol = area;

    // Each column: three equal-height panels, so all six bottoms align and lighter
    // panels (INPUT, DRIVE) keep their extra space as breathing room.
    auto splitThirds = [](juce::Rectangle<int> col) {
        const int h = (col.getHeight() - 2 * kRowGap) / 3;
        std::array<juce::Rectangle<int>, 3> r;
        r[0] = col.removeFromTop(h); col.removeFromTop(kRowGap);
        r[1] = col.removeFromTop(h); col.removeFromTop(kRowGap);
        r[2] = col;
        return r;
    };
    auto L = splitThirds(leftCol);
    input_.setBounds(L[0]); filter_.setBounds(L[1]); eq_.setBounds(L[2]);
    auto R = splitThirds(rightCol);
    comp_.setBounds(R[0]); drive_.setBounds(R[1]); matrix_.setBounds(R[2]);

    // Bridge interior: title (painted) on top, GR readout at the bottom, two ladders fill.
    auto br = bridgeArea_.reduced(8);
    br.removeFromTop(kPanelTitleH - 6);
    grReadout_.setBounds(br.removeFromBottom(18));
    br.removeFromBottom(8);
    const int half = br.getWidth() / 2;
    satMeter_.setBounds(br.removeFromLeft(half).reduced(3, 0));
    grMeter_.setBounds(br.reduced(3, 0));
}
```

- [ ] **Step 4: Add the missing include for `std::array` / GlyphArrangement**

At the top of `Source/PluginEditor.cpp`, ensure these includes exist after `#include "PluginEditor.h"`:

```cpp
#include <array>
```
(GlyphArrangement and MathConstants come in via the JUCE modules already included by `PluginEditor.h`.)

- [ ] **Step 5: Build**

Run: `cmake --build build --target Holdover -j`
Expected: builds successfully.

- [ ] **Step 6: Regression guard — run the DSP tests**

Run: `ctest --test-dir build --output-on-failure`
Expected: all tests PASS.

- [ ] **Step 7: Commit**

```bash
git add Source/ui/LedMeter.cpp Source/PluginEditor.h Source/PluginEditor.cpp
git commit -m "feat(ui): console layout — header wordmark, two columns, meter bridge"
```

---

## Task 8: Clean reinstall and manual visual verification

Produces a fresh installed build and verifies the rebuilt UI by eye against the spec. This is the real acceptance gate for a visual change.

**Files:** none (build + manual).

- [ ] **Step 1: Remove the currently-installed plugins**

```bash
rm -rf ~/Library/Audio/Plug-Ins/VST3/Holdover.vst3 ~/Library/Audio/Plug-Ins/Components/Holdover.component
```

- [ ] **Step 2: Clean-build and reinstall (COPY_PLUGIN_AFTER_BUILD reinstalls automatically)**

```bash
cmake --build build --target Holdover --clean-first -j
ls -la ~/Library/Audio/Plug-Ins/VST3/Holdover.vst3 ~/Library/Audio/Plug-Ins/Components/Holdover.component
```
Expected: build succeeds; both bundles exist with a fresh timestamp.

- [ ] **Step 3: Validate the format/host contract**

```bash
PV=$(ls build/**/pluginval* 2>/dev/null | head -1)
# If pluginval isn't in the build tree, skip — CI runs it at strictness 10.
[ -n "$PV" ] && "$PV" --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/Holdover.vst3 || echo "pluginval not local; rely on CI"
```
Expected: PASS, or deferred to CI.

- [ ] **Step 4: Manual visual check (open in a host — e.g. AudioPluginHost, Logic, Reaper)**

Confirm each, against `docs/superpowers/specs/2026-06-02-ui-rebuild-design.md`:
- [ ] Opens at **1020×680**, not shrunk.
- [ ] Dragging a corner scales the **whole UI uniformly**, locked to 3:2 — nothing stretches or overlaps; min ~714×476, max ~1632×1088.
- [ ] **Left column** = INPUT, FILTERS, EQ; **right column** = COMPRESSOR, DRIVE, MATRIX/OUTPUT; **center bridge** = two ladders + GR readout; header shows **HOLD**+accent **OVER**.
- [ ] Knobs are visibly larger and **identical** in size; the first knob columns line up vertically down each side; both columns bottom-align.
- [ ] **FILTER IN / EQ IN / COMP IN** render as sliding pill switches; **RMS / SC FILTER / SAT / HEX / CURVE / CEILING** as LED buttons that light on toggle; **MUTE** buttons glow red when active.
- [ ] Switching any toggle and turning any knob changes the bound parameter (automation/host shows the change); meters animate with signal.

- [ ] **Step 5: Capture a screenshot for sign-off**

Save a screenshot of the open plugin to `docs/superpowers/plans/ui-rebuild-screenshot.png` (or attach in the PR). This is the artifact that proves the visual result.

- [ ] **Step 6: Final commit (if any tuning was needed in Step 4)**

```bash
git add -A
git commit -m "chore(ui): visual verification tuning for console rebuild"
```

---

## Self-Review (completed by plan author)

**Spec coverage:**
- Strict alignment (uniform cells, aligned columns, bottom-align) → Tasks 2, 5, 6, 7. ✓
- Proportional resize / locked aspect → Task 1. ✓
- Opens full-size / initial-size ordering → Task 1 (Step 3). ✓
- Resize write cost stays cheap → unchanged plain `uiWidth/uiHeight` assignment in the existing `HoldoverEditor::resized()` (no change needed; it is already a plain assignment — confirmed at `PluginEditor.cpp:84-85`). ✓
- Two-column layout + center bridge + header (no tagline) → Task 7. ✓
- Pill switches / LED buttons / red mute → Tasks 3, 4, 5, 6. ✓
- Bigger uniform knobs → Tasks 2, 3 (rotary scales with cell). ✓
- Panel→control mapping unchanged → Tasks 5, 6 keep every parameter ID. ✓
- No DSP/parameter/preset change → only `Source/ui/**`, `PluginEditor.*`, and the two default-size constants in `PluginProcessor.h` are touched; `ctest` guards every task. ✓
- Bridge meters (SATURATING, COMPRESSING, GR readout) → Task 7. ✓

**Placeholder scan:** No TBD/TODO; every code step shows full code. The one intentional "red build" window (Tasks 3–5) is called out explicitly with rationale.

**Type consistency:** `ToggleStyle` (enum) defined in Task 3, consumed in Tasks 4/5/6. `ui::cells`, `ui::kCellH`, etc. defined in Task 2, used in Tasks 5/6/7. `AttachedToggle::attach(..., ToggleStyle)` 5-arg signature defined in Task 3 and every call site updated in Tasks 5/6. `headerArea_`/`bridgeArea_` declared in Task 7 Step 2, used in Step 3. Consistent. ✓
