#include "PluginEditor.h"

namespace holdover {

HoldoverEditor::HoldoverEditor(HoldoverProcessor& p)
    : AudioProcessorEditor(&p), processor(p), generic(p) {
    addAndMakeVisible(generic);
    setResizable(true, true);
    setResizeLimits(480, 360, 1600, 1200);
    setSize(720, 560);
}

void HoldoverEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff141416));
}

void HoldoverEditor::resized() {
    generic.setBounds(getLocalBounds());
}

} // namespace holdover
