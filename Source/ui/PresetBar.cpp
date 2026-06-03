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
