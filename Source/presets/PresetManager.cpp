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

void PresetManager::saveUser(const juce::String& name) {
    // Sanitize so a name with path separators / ".." can't write outside the dir.
    const juce::String safe = juce::File::createLegalFileName(name);
    if (safe.isEmpty()) return;
    const auto f = dir_.getChildFile(safe + ".preset");
    if (auto xml = apvts_.copyState().createXml())
        xml->writeTo(f, {});
    loadByName(safe);   // re-scan, select the just-saved preset, and markClean()
}

} // namespace holdover
