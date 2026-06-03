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
