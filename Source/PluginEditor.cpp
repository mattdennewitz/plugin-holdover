#include "PluginEditor.h"

namespace holdover {

HoldoverEditor::HoldoverEditor(HoldoverProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      input_(p.apvts), filter_(p.apvts), eq_(p.apvts),
      comp_(p.apvts), drive_(p.apvts), matrix_(p.apvts) {
    setLookAndFeel(&lnf_);
    for (auto* c : { (juce::Component*) &input_, (juce::Component*) &filter_,
                     (juce::Component*) &eq_, (juce::Component*) &comp_,
                     (juce::Component*) &drive_, (juce::Component*) &matrix_,
                     (juce::Component*) &satMeter_, (juce::Component*) &grMeter_ })
        addAndMakeVisible(c);

    grReadout_.setJustificationType(juce::Justification::centred);
    grReadout_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(grReadout_);

    setResizable(true, true);
    setResizeLimits(720, 520, 1600, 1100);
    getConstrainer()->setFixedAspectRatio(720.0 / 560.0);
    setSize(processor.uiWidth, processor.uiHeight);
    startTimerHz(30);
}

HoldoverEditor::~HoldoverEditor() { setLookAndFeel(nullptr); }

void HoldoverEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(HoldoverLookAndFeel::kBg));
}

void HoldoverEditor::resized() {
    auto area = getLocalBounds().reduced(8);
    auto meterCol = area.removeFromRight(140);

    // Signal-order panels stacked vertically; share remaining width.
    auto rowH = area.getHeight() / 3;
    auto top = area.removeFromTop(rowH);
    auto mid = area.removeFromTop(rowH);
    auto bot = area;

    auto topCols = Panel::columns(top, 2);
    input_.setBounds(topCols[0]); filter_.setBounds(topCols[1]);
    eq_.setBounds(mid);
    auto botCols = Panel::columns(bot, 2);
    comp_.setBounds(botCols[0]);
    // Drive on top of Matrix in the remaining column.
    auto dr = botCols[1].removeFromTop(botCols[1].getHeight() / 2);
    drive_.setBounds(dr);
    matrix_.setBounds(botCols[1]);

    // Meters column.
    auto satA = meterCol.removeFromTop(meterCol.getHeight() / 2).reduced(4);
    satMeter_.setBounds(satA);
    auto grA = meterCol.reduced(4);
    grReadout_.setBounds(grA.removeFromBottom(18));
    grMeter_.setBounds(grA);

    processor.uiWidth = getWidth();
    processor.uiHeight = getHeight();
}

void HoldoverEditor::timerCallback() {
    const auto& m = processor.getMeters();
    satMeter_.setLevel(m.saturation());
    const float grDb = m.gainReductionDb();
    grMeter_.setLevel(juce::jlimit(0.0f, 1.0f, grDb / 20.0f)); // 0..20 dB ladder
    grReadout_.setText(juce::String(grDb, 1) + " dB", juce::dontSendNotification);
}

} // namespace holdover
