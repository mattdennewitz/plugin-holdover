# In-UI Preset Selector Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an in-editor preset bar that browses/loads the existing factory presets and saves/recalls user presets from disk, mirroring the `plugin-treibend` / `plugin-denneverb` pattern.

**Architecture:** A non-UI `PresetManager` (in the headless `HoldoverDSP` lib) owns a combined factory+user list, loads by a single index space, and saves the whole APVTS state to `*.preset` files under `~/Library/Application Support/Holdover/Presets`. The processor delegates its host program methods to the manager and persists the current preset name. A `PresetBar` component in the editor header drives the manager (combo + prev/next + SAVE) and shows an unsaved-tweak `*` marker, polled from the existing editor timer.

**Tech Stack:** C++20, JUCE 8.0.4, CMake (Unix Makefiles), Catch2 v3 for tests.

---

## File Structure

- **Create** `Source/presets/PresetManager.h` — manager interface.
- **Create** `Source/presets/PresetManager.cpp` — manager implementation (added to `HoldoverDSP`).
- **Create** `Source/ui/PresetBar.h` / `Source/ui/PresetBar.cpp` — header preset bar component.
- **Create** `tests/test_presetmanager.cpp` — headless manager tests (in `HoldoverTests`).
- **Create** `tests/test_processor_presets.cpp` — processor delegation + persistence tests (in `HoldoverEditorTests`).
- **Modify** `Source/CMakeLists.txt` — register `PresetManager.cpp` (lib) and `PresetBar.cpp` (plugin).
- **Modify** `tests/CMakeLists.txt` — register the two new test files + `PresetBar.cpp` in the editor test exe.
- **Modify** `Source/PluginProcessor.h` / `.cpp` — own the manager, delegate programs, persist `currentPresetName`.
- **Modify** `Source/PluginEditor.h` / `.cpp` — host the `PresetBar` in the header, sync from the timer.

**Build/test commands used throughout** (run from repo root `/Users/matt/src/plugin-holdover`):
- Reconfigure after any `CMakeLists.txt` edit: `cmake -B build`
- Build headless tests: `cmake --build build --target HoldoverTests -j`
- Run a tag subset: `./build/tests/HoldoverTests "[presetmanager]"`
- Build editor tests: `cmake --build build --target HoldoverEditorTests -j`
- Build the plugin: `cmake --build build --target Holdover -j`

---

## Task 1: `PresetManager` — factory list + selection

**Files:**
- Create: `Source/presets/PresetManager.h`
- Create: `Source/presets/PresetManager.cpp`
- Create: `tests/test_presetmanager.cpp`
- Modify: `Source/CMakeLists.txt` (add `presets/PresetManager.cpp` to `HoldoverDSP`)
- Modify: `tests/CMakeLists.txt` (add `test_presetmanager.cpp` to `HoldoverTests`)

- [ ] **Step 1: Create the header**

Create `Source/presets/PresetManager.h`:

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FactoryPresets.h"

namespace holdover {

// Owns the combined factory + user preset list in a single index space:
// indices [0, getNumFactory()) are factory presets; the rest are user files
// (*.preset) read from `dir`. All disk I/O happens here, on the message thread.
class PresetManager {
public:
    PresetManager(juce::AudioProcessorValueTreeState& apvtsToUse, juce::File presetDir);
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvtsToUse); // default user dir

    juce::StringArray getAllNames() const;        // factory names, then user file names (sorted)
    int  getNumFactory() const { return (int) presets::all().size(); }
    void loadByIndex(int index);
    void loadByName(const juce::String& name);
    void saveUser(const juce::String& name);      // writes <dir>/<legal-name>.preset, then selects it
    void next();
    void prev();
    int  getCurrentIndex() const { return currentIndex_; }
    juce::String getCurrentName() const;
    bool isModified() const;                       // current state differs from last load/save

    static juce::File defaultDir();

private:
    void markClean();                              // snapshot current state as the reference
    juce::Array<juce::File> userFiles() const;     // *.preset sorted by name

    juce::AudioProcessorValueTreeState& apvts_;
    juce::File      dir_;
    juce::ValueTree referenceState_;               // state at the last load/save; for isModified()
    int  currentIndex_ = 0;
};

} // namespace holdover
```

- [ ] **Step 2: Create the implementation (factory paths only for now)**

Create `Source/presets/PresetManager.cpp`:

```cpp
#include "PresetManager.h"

namespace holdover {

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& a, juce::File presetDir)
    : apvts_(a), dir_(std::move(presetDir)) {
    if (! dir_.isDirectory()) dir_.createDirectory();
    markClean();
}

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& a)
    : PresetManager(a, defaultDir()) {}

juce::File PresetManager::defaultDir() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("Holdover").getChildFile("Presets");
}

juce::Array<juce::File> PresetManager::userFiles() const {
    auto files = dir_.findChildFiles(juce::File::findFiles, false, "*.preset");
    files.sort();
    return files;
}

juce::StringArray PresetManager::getAllNames() const {
    juce::StringArray names;
    for (const auto& p : presets::all()) names.add(p.name);
    for (const auto& f : userFiles())    names.add(f.getFileNameWithoutExtension());
    return names;
}

void PresetManager::loadByIndex(int index) {
    const auto names = getAllNames();
    if (names.isEmpty()) return;
    index = juce::jlimit(0, names.size() - 1, index);
    currentIndex_ = index;

    if (index < getNumFactory()) {
        presets::apply(apvts_, index);
    } else {
        const auto f = dir_.getChildFile(names[index] + ".preset");
        if (auto xml = juce::XmlDocument::parse(f)) {
            const auto tree = juce::ValueTree::fromXml(*xml);
            apvts_.replaceState(tree);
            // Force-apply each stored value so snapped bool/choice params recall
            // exactly: replaceState skips params whose raw float delta is ~0
            // (the same quirk handled in HoldoverProcessor::setStateInformation).
            for (int i = 0; i < tree.getNumChildren(); ++i) {
                const auto child = tree.getChild(i);
                if (auto* param = apvts_.getParameter(child.getProperty("id").toString())) {
                    const float stored = child.getProperty("value", param->getDefaultValue());
                    param->setValueNotifyingHost(param->convertTo0to1(stored));
                }
            }
        }
    }
    markClean();
}

void PresetManager::loadByName(const juce::String& name) {
    const int idx = getAllNames().indexOf(name);
    if (idx >= 0) loadByIndex(idx);
}

void PresetManager::next() { loadByIndex(currentIndex_ + 1); }
void PresetManager::prev() { loadByIndex(currentIndex_ - 1); }

juce::String PresetManager::getCurrentName() const {
    const auto names = getAllNames();
    return juce::isPositiveAndBelow(currentIndex_, names.size()) ? names[currentIndex_]
                                                                 : juce::String();
}

void PresetManager::markClean() { referenceState_ = apvts_.copyState(); }

bool PresetManager::isModified() const {
    return ! apvts_.copyState().isEquivalentTo(referenceState_);
}

// saveUser() is implemented in Task 2.
void PresetManager::saveUser(const juce::String&) {}

} // namespace holdover
```

- [ ] **Step 3: Register the source in `HoldoverDSP`**

In `Source/CMakeLists.txt`, the `add_library(HoldoverDSP STATIC ...)` list currently ends:

```cmake
    dsp/ChannelStrip.cpp
    # dsp/*.cpp added by later tasks
    presets/FactoryPresets.cpp
)
```

Add `presets/PresetManager.cpp` right after `FactoryPresets.cpp`:

```cmake
    dsp/ChannelStrip.cpp
    # dsp/*.cpp added by later tasks
    presets/FactoryPresets.cpp
    presets/PresetManager.cpp
)
```

- [ ] **Step 4: Register the test file in `HoldoverTests`**

In `tests/CMakeLists.txt`, add `test_presetmanager.cpp` to the `add_executable(HoldoverTests ...)` list (after `test_presets.cpp`):

```cmake
    test_presets.cpp
    test_presetmanager.cpp
    test_hardening.cpp)
```

- [ ] **Step 5: Write the failing tests**

Create `tests/test_presetmanager.cpp`:

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "presets/PresetManager.h"
#include "Parameters.h"

using Catch::Approx;

namespace {

// Minimal AudioProcessor purely to host an APVTS in a headless test.
struct StubProcessor : juce::AudioProcessor {
    juce::AudioProcessorValueTreeState apvts {
        *this, nullptr, "Parameters", holdover::params::createLayout() };

    StubProcessor() : juce::AudioProcessor(BusesProperties()) {}
    const juce::String getName() const override { return "Stub"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

float raw(juce::AudioProcessorValueTreeState& a, const char* id) {
    return a.getRawParameterValue(id)->load();
}

void setReal(juce::AudioProcessorValueTreeState& a, const char* id, float real) {
    auto* p = a.getParameter(id);
    p->setValueNotifyingHost(p->convertTo0to1(real));
}

// Fresh empty temp dir, unique per call.
juce::File freshDir() {
    auto d = juce::File::getSpecialLocation(juce::File::tempDirectory)
                 .getChildFile("holdover_pm_" + juce::String(juce::Time::getMillisecondCounter())
                               + "_" + juce::String(juce::Random::getSystemRandom().nextInt(99999)));
    d.deleteRecursively();
    d.createDirectory();
    return d;
}

} // namespace

TEST_CASE("factory list is exposed before any user presets", "[presetmanager]") {
    StubProcessor sp;
    auto dir = freshDir();
    holdover::PresetManager pm(sp.apvts, dir);

    REQUIRE(pm.getNumFactory() == (int) holdover::presets::all().size());
    const auto names = pm.getAllNames();
    REQUIRE(names.size() == pm.getNumFactory());
    REQUIRE(names[0] == "Init / Flat");
    REQUIRE(pm.getCurrentIndex() == 0);
    REQUIRE(pm.getCurrentName() == "Init / Flat");

    dir.deleteRecursively();
}

TEST_CASE("loadByIndex applies a factory preset to the parameters", "[presetmanager]") {
    StubProcessor sp;
    auto dir = freshDir();
    holdover::PresetManager pm(sp.apvts, dir);

    pm.loadByIndex(3); // "Bass Saturator" sets drive=8
    REQUIRE(pm.getCurrentIndex() == 3);
    REQUIRE(pm.getCurrentName() == "Bass Saturator");
    REQUIRE(raw(sp.apvts, "drive") == Approx(8.0f));

    dir.deleteRecursively();
}

TEST_CASE("next and prev clamp at the ends", "[presetmanager]") {
    StubProcessor sp;
    auto dir = freshDir();
    holdover::PresetManager pm(sp.apvts, dir);

    pm.loadByIndex(0);
    pm.prev();
    REQUIRE(pm.getCurrentIndex() == 0);

    const int last = pm.getNumFactory() - 1;
    pm.loadByIndex(last);
    pm.next();
    REQUIRE(pm.getCurrentIndex() == last);

    dir.deleteRecursively();
}
```

- [ ] **Step 6: Reconfigure, build, and verify the tests pass**

Run: `cmake -B build && cmake --build build --target HoldoverTests -j && ./build/tests/HoldoverTests "[presetmanager]"`
Expected: build succeeds; 3 test cases pass (`All tests passed`).

- [ ] **Step 7: Commit**

```bash
git add Source/presets/PresetManager.h Source/presets/PresetManager.cpp \
        tests/test_presetmanager.cpp Source/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(presets): add PresetManager with factory selection"
```

---

## Task 2: `PresetManager` — user save + load

**Files:**
- Modify: `Source/presets/PresetManager.cpp` (replace the stub `saveUser`)
- Modify: `tests/test_presetmanager.cpp` (add user-preset tests)

- [ ] **Step 1: Write the failing tests**

Append these test cases to the end of `tests/test_presetmanager.cpp`:

```cpp
TEST_CASE("saveUser writes a .preset file and selects it", "[presetmanager]") {
    StubProcessor sp;
    auto dir = freshDir();
    holdover::PresetManager pm(sp.apvts, dir);

    pm.saveUser("My Sound");
    REQUIRE(dir.getChildFile("My Sound.preset").existsAsFile());
    REQUIRE(pm.getAllNames().contains("My Sound"));
    REQUIRE(pm.getCurrentName() == "My Sound");
    REQUIRE_FALSE(pm.isModified());

    dir.deleteRecursively();
}

TEST_CASE("user preset round-trips snapped bool and choice params", "[presetmanager]") {
    StubProcessor sp;
    auto dir = freshDir();
    holdover::PresetManager pm(sp.apvts, dir);

    // A distinctive mix including a bool (compMute) and a choice (masMode).
    setReal(sp.apvts, "character", 7.0f);
    sp.apvts.getParameter("compMute")->setValueNotifyingHost(0.0f); // false
    sp.apvts.getParameter("masMode")->setValueNotifyingHost(1.0f);  // last option
    const float c    = raw(sp.apvts, "character");
    const float mute = raw(sp.apvts, "compMute");
    const float mas  = raw(sp.apvts, "masMode");

    pm.saveUser("Round");

    // Disturb each value explicitly (don't rely on a factory preset touching them).
    setReal(sp.apvts, "character", 0.0f);
    sp.apvts.getParameter("compMute")->setValueNotifyingHost(1.0f); // true
    sp.apvts.getParameter("masMode")->setValueNotifyingHost(0.0f);  // first option
    REQUIRE(raw(sp.apvts, "character") == Approx(0.0f));

    pm.loadByName("Round");         // recall via the user-file force-apply path
    REQUIRE(raw(sp.apvts, "character") == Approx(c));
    REQUIRE(raw(sp.apvts, "compMute")  == Approx(mute));
    REQUIRE(raw(sp.apvts, "masMode")   == Approx(mas));

    dir.deleteRecursively();
}

TEST_CASE("saving the same name overwrites in place", "[presetmanager]") {
    StubProcessor sp;
    auto dir = freshDir();
    holdover::PresetManager pm(sp.apvts, dir);

    setReal(sp.apvts, "character", 2.0f);
    pm.saveUser("Dup");
    setReal(sp.apvts, "character", 9.0f);
    pm.saveUser("Dup");

    REQUIRE(dir.findChildFiles(juce::File::findFiles, false, "Dup.preset").size() == 1);

    setReal(sp.apvts, "character", 0.0f);   // disturb, then prove the file holds 9
    pm.loadByName("Dup");
    REQUIRE(raw(sp.apvts, "character") == Approx(9.0f));

    dir.deleteRecursively();
}

TEST_CASE("illegal and empty names are handled safely", "[presetmanager]") {
    StubProcessor sp;
    auto dir = freshDir();
    holdover::PresetManager pm(sp.apvts, dir);

    pm.saveUser("");                // nothing written
    REQUIRE(dir.findChildFiles(juce::File::findFiles, false, "*.preset").isEmpty());

    pm.saveUser("a/b");             // legalized, stays inside dir
    const auto legal = juce::File::createLegalFileName("a/b");
    REQUIRE(dir.getChildFile(legal + ".preset").existsAsFile());
    REQUIRE(pm.getAllNames().contains(legal));

    dir.deleteRecursively();
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target HoldoverTests -j && ./build/tests/HoldoverTests "[presetmanager]"`
Expected: the new cases FAIL (the stub `saveUser` writes nothing, so file-existence and round-trip assertions fail).

- [ ] **Step 3: Implement `saveUser`**

In `Source/presets/PresetManager.cpp`, replace the stub line:

```cpp
// saveUser() is implemented in Task 2.
void PresetManager::saveUser(const juce::String&) {}
```

with the real implementation:

```cpp
void PresetManager::saveUser(const juce::String& name) {
    // Sanitize so a name with path separators / ".." can't write outside the dir.
    const juce::String safe = juce::File::createLegalFileName(name);
    if (safe.isEmpty()) return;
    const auto f = dir_.getChildFile(safe + ".preset");
    if (auto xml = apvts_.copyState().createXml())
        xml->writeTo(f, {});
    loadByName(safe);   // re-scan, select the just-saved preset, and markClean()
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `cmake --build build --target HoldoverTests -j && ./build/tests/HoldoverTests "[presetmanager]"`
Expected: all 7 `[presetmanager]` cases PASS.

- [ ] **Step 5: Commit**

```bash
git add Source/presets/PresetManager.cpp tests/test_presetmanager.cpp
git commit -m "feat(presets): add user preset save/load to PresetManager"
```

---

## Task 3: Processor integration

**Files:**
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`
- Create: `tests/test_processor_presets.cpp`
- Modify: `tests/CMakeLists.txt` (add the test to `HoldoverEditorTests`)

- [ ] **Step 1: Add the manager + name to the processor header**

In `Source/PluginProcessor.h`, add the include near the top (after the existing `presets/FactoryPresets.h`):

```cpp
#include "presets/FactoryPresets.h"
#include "presets/PresetManager.h"
```

Add the public accessor + persisted name. Replace:

```cpp
    juce::AudioProcessorValueTreeState apvts;

    int uiWidth = 1020, uiHeight = 680;
```

with:

```cpp
    juce::AudioProcessorValueTreeState apvts;

    PresetManager& getPresetManager() { return presetManager; }
    juce::String currentPresetName;     // persisted; set by the editor on load/save

    int uiWidth = 1020, uiHeight = 680;
```

In the `private:` section, replace:

```cpp
    ChannelStrip strip_;
    int currentProgram_ = 0;
```

with (note: `presetManager` is declared AFTER `apvts`, which it references, so construction order is safe — `apvts` is declared above in the public section):

```cpp
    ChannelStrip strip_;
    PresetManager presetManager { apvts };   // depends on apvts (declared earlier)
```

- [ ] **Step 2: Default the name + delegate the program methods**

In `Source/PluginProcessor.cpp`, set the default name at the end of the constructor. Replace:

```cpp
      apvts(*this, nullptr, "Parameters", params::createLayout()) {}
```

with:

```cpp
      apvts(*this, nullptr, "Parameters", params::createLayout()) {
    currentPresetName = presetManager.getCurrentName();   // "Init / Flat" on first launch
}
```

Replace the program block:

```cpp
int HoldoverProcessor::getNumPrograms() { return (int) presets::all().size(); }
int HoldoverProcessor::getCurrentProgram() { return currentProgram_; }

void HoldoverProcessor::setCurrentProgram(int index) {
    index = juce::jlimit(0, getNumPrograms() - 1, index);
    if (index == currentProgram_) return;
    currentProgram_ = index;
    presets::apply(apvts, currentProgram_);
}

const juce::String HoldoverProcessor::getProgramName(int index) {
    if (index >= 0 && index < getNumPrograms())
        return presets::all()[(size_t) index].name;
    return {};
}
```

with delegation to the manager (single source of truth for the current index):

```cpp
int HoldoverProcessor::getNumPrograms() { return presetManager.getNumFactory(); }

int HoldoverProcessor::getCurrentProgram() {
    // The host menu is factory-only; a loaded user preset reports as the clamped
    // factory index.
    return juce::jlimit(0, getNumPrograms() - 1, presetManager.getCurrentIndex());
}

void HoldoverProcessor::setCurrentProgram(int index) {
    presetManager.loadByIndex(index);
    currentPresetName = presetManager.getCurrentName();
}

const juce::String HoldoverProcessor::getProgramName(int index) {
    if (index >= 0 && index < getNumPrograms())
        return presets::all()[(size_t) index].name;
    return {};
}
```

- [ ] **Step 3: Persist `currentPresetName` in plugin state**

In `Source/PluginProcessor.cpp`, in `getStateInformation`, the `UI` child is built like this:

```cpp
    juce::ValueTree ui("UI");
    ui.setProperty("width", uiWidth, nullptr);
    ui.setProperty("height", uiHeight, nullptr);
```

Add the preset name:

```cpp
    juce::ValueTree ui("UI");
    ui.setProperty("width", uiWidth, nullptr);
    ui.setProperty("height", uiHeight, nullptr);
    ui.setProperty("preset", currentPresetName, nullptr);
```

In `setStateInformation`, the UI restore block reads:

```cpp
    auto ui = root.getChildWithName("UI");
    if (ui.isValid()) {
        uiWidth  = (int) ui.getProperty("width", uiWidth);
        uiHeight = (int) ui.getProperty("height", uiHeight);
    }
```

Add the preset name restore:

```cpp
    auto ui = root.getChildWithName("UI");
    if (ui.isValid()) {
        uiWidth  = (int) ui.getProperty("width", uiWidth);
        uiHeight = (int) ui.getProperty("height", uiHeight);
        currentPresetName = ui.getProperty("preset", currentPresetName).toString();
    }
```

- [ ] **Step 4: Register a processor test in `HoldoverEditorTests`**

In `tests/CMakeLists.txt`, the editor test executable is:

```cmake
add_executable(HoldoverEditorTests
    test_editor_size.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/HoldoverLookAndFeel.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/LedMeter.cpp)
```

Add `test_processor_presets.cpp` as the second source:

```cmake
add_executable(HoldoverEditorTests
    test_editor_size.cpp
    test_processor_presets.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/HoldoverLookAndFeel.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/LedMeter.cpp)
```

- [ ] **Step 5: Write the failing processor tests**

Create `tests/test_processor_presets.cpp`:

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "PluginProcessor.h"
#include "presets/FactoryPresets.h"

using Catch::Approx;

TEST_CASE("host program interface delegates to the preset manager", "[processor]") {
    holdover::HoldoverProcessor proc;

    REQUIRE(proc.getNumPrograms() == (int) holdover::presets::all().size());

    proc.setCurrentProgram(3); // "Bass Saturator" -> drive=8
    REQUIRE(proc.getCurrentProgram() == 3);
    REQUIRE(proc.getProgramName(3) == "Bass Saturator");
    REQUIRE(proc.apvts.getRawParameterValue("drive")->load() == Approx(8.0f));
    REQUIRE(proc.getPresetManager().getCurrentIndex() == 3);
}

TEST_CASE("currentPresetName defaults to the first factory preset", "[processor]") {
    holdover::HoldoverProcessor proc;
    REQUIRE(proc.currentPresetName == "Init / Flat");
}

TEST_CASE("currentPresetName round-trips through plugin state", "[processor]") {
    holdover::HoldoverProcessor a;
    a.currentPresetName = "Vocal Glue";

    juce::MemoryBlock mb;
    a.getStateInformation(mb);

    holdover::HoldoverProcessor b;
    b.setStateInformation(mb.getData(), (int) mb.getSize());
    REQUIRE(b.currentPresetName == "Vocal Glue");
}
```

- [ ] **Step 6: Reconfigure, build, and verify**

Run: `cmake -B build && cmake --build build --target HoldoverEditorTests -j && ./build/tests/HoldoverEditorTests "[processor]"`
Expected: build succeeds; 3 `[processor]` cases PASS.

- [ ] **Step 7: Verify the headless suite still builds (no `currentProgram_` left dangling)**

Run: `cmake --build build --target HoldoverTests -j && ./build/tests/HoldoverTests`
Expected: all tests PASS.

- [ ] **Step 8: Commit**

```bash
git add Source/PluginProcessor.h Source/PluginProcessor.cpp \
        tests/test_processor_presets.cpp tests/CMakeLists.txt
git commit -m "feat(presets): delegate host programs to PresetManager and persist preset name"
```

---

## Task 4: `PresetBar` component

**Files:**
- Create: `Source/ui/PresetBar.h`
- Create: `Source/ui/PresetBar.cpp`
- Modify: `Source/CMakeLists.txt` (add `ui/PresetBar.cpp` to the `Holdover` plugin target)
- Modify: `tests/CMakeLists.txt` (add `ui/PresetBar.cpp` to `HoldoverEditorTests`)

There is no unit test for UI (consistent with the rest of the suite); the verification step is a successful compile/link into the editor test executable.

- [ ] **Step 1: Create the header**

Create `Source/ui/PresetBar.h`:

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"

namespace holdover {

// Header preset bar: [ combo ▾ ] [*] [ < ] [ > ] [ SAVE ]. Drives the processor's
// PresetManager. The modified marker and combo selection are refreshed from the
// editor timer via syncSelection() (message thread only — no APVTS listener, so
// no audio-thread UI hazard).
class PresetBar : public juce::Component {
public:
    explicit PresetBar(HoldoverProcessor&);

    void resized() override;
    void syncSelection();   // called each editor timer tick

private:
    void refreshList();
    void showSaveDialog();

    HoldoverProcessor& processor_;
    juce::ComboBox   presetBox_;
    juce::Label      modLabel_;
    juce::TextButton prevBtn_, nextBtn_, saveBtn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBar)
};

} // namespace holdover
```

- [ ] **Step 2: Create the implementation**

Create `Source/ui/PresetBar.cpp`:

```cpp
#include "PresetBar.h"

namespace holdover {

PresetBar::PresetBar(HoldoverProcessor& p) : processor_(p) {
    addAndMakeVisible(presetBox_);
    presetBox_.onChange = [this] {
        const int idx = presetBox_.getSelectedItemIndex();
        if (idx < 0) return;
        auto& pm = processor_.getPresetManager();
        pm.loadByIndex(idx);
        processor_.currentPresetName = pm.getCurrentName();
    };

    prevBtn_.setButtonText("<");
    nextBtn_.setButtonText(">");
    saveBtn_.setButtonText("SAVE");
    addAndMakeVisible(prevBtn_);
    addAndMakeVisible(nextBtn_);
    addAndMakeVisible(saveBtn_);

    auto step = [this](bool forward) {
        auto& pm = processor_.getPresetManager();
        forward ? pm.next() : pm.prev();
        processor_.currentPresetName = pm.getCurrentName();
        refreshList();
    };
    prevBtn_.onClick = [step] { step(false); };
    nextBtn_.onClick = [step] { step(true); };
    saveBtn_.onClick = [this] { showSaveDialog(); };

    modLabel_.setText("*", juce::dontSendNotification);
    modLabel_.setJustificationType(juce::Justification::centred);
    modLabel_.setInterceptsMouseClicks(false, false);
    modLabel_.setVisible(false);
    addAndMakeVisible(modLabel_);

    refreshList();
}

void PresetBar::refreshList() {
    auto& pm = processor_.getPresetManager();
    presetBox_.clear(juce::dontSendNotification);
    const auto names = pm.getAllNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox_.addItem(names[i], i + 1);
    presetBox_.setSelectedItemIndex(pm.getCurrentIndex(), juce::dontSendNotification);
}

void PresetBar::showSaveDialog() {
    auto* aw = new juce::AlertWindow("Save Preset", "Preset name:",
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", processor_.getPresetManager().getCurrentName());
    aw->addButton("Save", 1);
    aw->addButton("Cancel", 0);

    juce::Component::SafePointer<PresetBar> safe(this);
    aw->enterModalState(true, juce::ModalCallbackFunction::create([safe, aw](int r) {
        // Guard against the editor (and this bar) being destroyed while the dialog is open.
        if (r == 1 && safe != nullptr) {
            const auto name = aw->getTextEditorContents("name").trim();
            if (name.isNotEmpty()) {
                safe->processor_.getPresetManager().saveUser(name);
                safe->processor_.currentPresetName = name;
                safe->refreshList();
            }
        }
        delete aw;
    }), false);
}

void PresetBar::syncSelection() {
    auto& pm = processor_.getPresetManager();
    if (presetBox_.getSelectedItemIndex() != pm.getCurrentIndex())
        presetBox_.setSelectedItemIndex(pm.getCurrentIndex(), juce::dontSendNotification);

    const bool mod = pm.isModified();
    if (modLabel_.isVisible() != mod)
        modLabel_.setVisible(mod);
}

void PresetBar::resized() {
    auto r = getLocalBounds();
    saveBtn_.setBounds(r.removeFromRight(52));
    r.removeFromRight(6);
    nextBtn_.setBounds(r.removeFromRight(26));
    prevBtn_.setBounds(r.removeFromRight(26));
    r.removeFromRight(6);
    modLabel_.setBounds(r.removeFromRight(14));
    presetBox_.setBounds(r);
}

} // namespace holdover
```

- [ ] **Step 3: Register `PresetBar.cpp` in the plugin target**

In `Source/CMakeLists.txt`, `target_sources(Holdover PRIVATE ...)` currently reads:

```cmake
target_sources(Holdover PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    ui/HoldoverLookAndFeel.cpp
    ui/LedMeter.cpp)
```

Add `ui/PresetBar.cpp`:

```cmake
target_sources(Holdover PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    ui/HoldoverLookAndFeel.cpp
    ui/LedMeter.cpp
    ui/PresetBar.cpp)
```

- [ ] **Step 4: Register `PresetBar.cpp` in `HoldoverEditorTests`**

In `tests/CMakeLists.txt`, add `${CMAKE_SOURCE_DIR}/Source/ui/PresetBar.cpp` to the `HoldoverEditorTests` sources (after `PluginEditor.cpp`):

```cmake
    ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/PresetBar.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/HoldoverLookAndFeel.cpp
    ${CMAKE_SOURCE_DIR}/Source/ui/LedMeter.cpp)
```

- [ ] **Step 5: Reconfigure and verify it compiles/links**

Run: `cmake -B build && cmake --build build --target HoldoverEditorTests -j`
Expected: compiles and links cleanly (the bar is not yet placed in the editor; this only proves it builds). `./build/tests/HoldoverEditorTests` still PASSES all existing cases.

- [ ] **Step 6: Commit**

```bash
git add Source/ui/PresetBar.h Source/ui/PresetBar.cpp Source/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): add PresetBar component"
```

---

## Task 5: Editor wiring

**Files:**
- Modify: `Source/PluginEditor.h`
- Modify: `Source/PluginEditor.cpp`

- [ ] **Step 1: Add the include + member to `Content`**

In `Source/PluginEditor.h`, add the include with the other panel includes:

```cpp
#include "ui/panels/MatrixPanel.h"
#include "ui/PresetBar.h"
```

Change the `Content` constructor signature. Replace:

```cpp
    class Content : public juce::Component {
    public:
        explicit Content(juce::AudioProcessorValueTreeState&);
        void paint(juce::Graphics&) override;
        void resized() override;
        void updateMeters(float saturation, float gainReductionDb);
```

with (adds a preset-sync hook and takes the processor so the bar can reach the manager):

```cpp
    class Content : public juce::Component {
    public:
        explicit Content(HoldoverProcessor&);
        void paint(juce::Graphics&) override;
        void resized() override;
        void updateMeters(float saturation, float gainReductionDb);
        void syncPresetBar();
```

Add the `PresetBar` member alongside the other panels (after `matrix_`):

```cpp
        DrivePanel drive_;
        MatrixPanel matrix_;
        PresetBar presetBar_;
```

- [ ] **Step 2: Update the `Content` constructor**

In `Source/PluginEditor.cpp`, replace the constructor:

```cpp
HoldoverEditor::Content::Content(juce::AudioProcessorValueTreeState& apvts)
    : input_(apvts), filter_(apvts), eq_(apvts),
      comp_(apvts), drive_(apvts), matrix_(apvts) {
    for (auto* c : { (juce::Component*) &input_, (juce::Component*) &filter_,
                     (juce::Component*) &eq_, (juce::Component*) &comp_,
                     (juce::Component*) &drive_, (juce::Component*) &matrix_,
                     (juce::Component*) &satMeter_, (juce::Component*) &grMeter_ })
        addAndMakeVisible(c);
```

with (panels now read `processor.apvts`; the bar takes the processor; the bar is added to the visible-children list):

```cpp
HoldoverEditor::Content::Content(HoldoverProcessor& processor)
    : input_(processor.apvts), filter_(processor.apvts), eq_(processor.apvts),
      comp_(processor.apvts), drive_(processor.apvts), matrix_(processor.apvts),
      presetBar_(processor) {
    for (auto* c : { (juce::Component*) &input_, (juce::Component*) &filter_,
                     (juce::Component*) &eq_, (juce::Component*) &comp_,
                     (juce::Component*) &drive_, (juce::Component*) &matrix_,
                     (juce::Component*) &satMeter_, (juce::Component*) &grMeter_,
                     (juce::Component*) &presetBar_ })
        addAndMakeVisible(c);
```

- [ ] **Step 3: Place the bar in the header in `resized()`**

In `Source/PluginEditor.cpp`, `Content::resized()` begins:

```cpp
    headerArea_ = area.removeFromTop(kHeaderH).reduced(4, 0);
    area.removeFromTop(kRowGap);
```

Insert the bar layout right after `headerArea_` is set (reserve the left for the wordmark, give the right to the bar):

```cpp
    headerArea_ = area.removeFromTop(kHeaderH).reduced(4, 0);
    {
        auto bar = headerArea_;
        bar.removeFromLeft(160);                 // reserve space for the HOLD OVER wordmark
        presetBar_.setBounds(bar.withSizeKeepingCentre(bar.getWidth(), kChoiceBoxH));
    }
    area.removeFromTop(kRowGap);
```

- [ ] **Step 4: Add the `syncPresetBar` method**

In `Source/PluginEditor.cpp`, after `Content::updateMeters(...)` add:

```cpp
void HoldoverEditor::Content::syncPresetBar() {
    presetBar_.syncSelection();
}
```

- [ ] **Step 5: Construct `Content` with the processor + sync from the timer**

In `Source/PluginEditor.cpp`, the editor constructor initializer list reads:

```cpp
HoldoverEditor::HoldoverEditor(HoldoverProcessor& p)
    : AudioProcessorEditor(&p), processor(p), content_(p.apvts) {
```

Change `content_(p.apvts)` to `content_(p)`:

```cpp
HoldoverEditor::HoldoverEditor(HoldoverProcessor& p)
    : AudioProcessorEditor(&p), processor(p), content_(p) {
```

The editor's `timerCallback` reads:

```cpp
void HoldoverEditor::timerCallback() {
    const auto& m = processor.getMeters();
    content_.updateMeters(m.saturation(), m.gainReductionDb());
}
```

Add the preset sync:

```cpp
void HoldoverEditor::timerCallback() {
    const auto& m = processor.getMeters();
    content_.updateMeters(m.saturation(), m.gainReductionDb());
    content_.syncPresetBar();
}
```

- [ ] **Step 6: Build the editor tests and the plugin**

Run: `cmake --build build --target HoldoverEditorTests -j && ./build/tests/HoldoverEditorTests`
Expected: builds; all cases (including the existing `test_editor_size` and the `[processor]` cases) PASS.

Run: `cmake --build build --target Holdover -j`
Expected: the VST3/AU plugin builds cleanly.

- [ ] **Step 7: Commit**

```bash
git add Source/PluginEditor.h Source/PluginEditor.cpp
git commit -m "feat(ui): mount preset bar in the editor header"
```

---

## Task 6: Full verification

- [ ] **Step 1: Clean-build every target and run the whole suite**

Run: `cmake -B build && cmake --build build -j && ctest --test-dir build --output-on-failure`
Expected: all targets build; every Catch2 test (DSP suite + editor suite) passes.

- [ ] **Step 2: Manual smoke check (optional, requires a host)**

Load the built plugin in a DAW or the JUCE AudioPluginHost:
- The header shows a preset combo defaulting to `Init / Flat`.
- Selecting a factory preset changes the sound; `<` / `>` step through the list.
- Move any knob → a `*` appears next to the combo within ~1 timer tick.
- `SAVE` → type a name → the preset appears in the `USER` portion of the list and the `*` clears.
- Reopen the editor / reload the project → the combo still shows the last preset name.
- Confirm a user `.preset` file exists under `~/Library/Application Support/Holdover/Presets`.

- [ ] **Step 3: No commit** (verification only).

---

## Notes / deviations from the spec

- **Modified indicator mechanism:** implemented as a timer-polled state comparison
  (`isModified()` compares `apvts.copyState()` to a reference snapshot taken at each
  load/save) rather than an `APVTS::Listener`. This keeps all modified-state work on
  the message thread; an APVTS listener can fire on the audio thread during host
  automation, where touching UI or non-atomic flags is unsafe. Same visible result.
- **`FACTORY` / `USER` section headings** in the combo are omitted: the combined index
  maps 1:1 to combo item indices, and a section heading consumes an item slot that would
  desync `getSelectedItemIndex()` from `getCurrentIndex()`. Left out to keep the index
  mapping trivial and correct (YAGNI).
- **Single source of truth:** the removed `currentProgram_` member is fully replaced by
  `PresetManager::currentIndex_`; the processor's program methods delegate, so host
  program changes and the in-UI combo never drift.
