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
