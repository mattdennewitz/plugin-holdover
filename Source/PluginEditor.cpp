#include "PluginEditor.h"

namespace holdover {

// ============================ Content =====================================

HoldoverEditor::Content::Content(juce::AudioProcessorValueTreeState& apvts)
    : input_(apvts), filter_(apvts), eq_(apvts),
      comp_(apvts), drive_(apvts), matrix_(apvts) {
    for (auto* c : { (juce::Component*) &input_, (juce::Component*) &filter_,
                     (juce::Component*) &eq_, (juce::Component*) &comp_,
                     (juce::Component*) &drive_, (juce::Component*) &matrix_,
                     (juce::Component*) &satMeter_, (juce::Component*) &grMeter_ })
        addAndMakeVisible(c);

    grReadout_.setJustificationType(juce::Justification::centred);
    grReadout_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(grReadout_);
}

// Fixed layout at the base design size. Every control receives its natural
// bounds here; the parent editor scales the whole component uniformly.
void HoldoverEditor::Content::resized() {
    auto area = getLocalBounds().reduced(8);
    auto meterCol = area.removeFromRight(140);

    const int rowH = area.getHeight() / 3;
    auto top = area.removeFromTop(rowH);
    auto mid = area.removeFromTop(rowH);
    auto bot = area;

    auto topCols = Panel::columns(top, 2);
    input_.setBounds(topCols[0]);
    filter_.setBounds(topCols[1]);
    eq_.setBounds(mid);

    auto botCols = Panel::columns(bot, 2);
    comp_.setBounds(botCols[0]);
    auto dr = botCols[1].removeFromTop(botCols[1].getHeight() / 2);
    drive_.setBounds(dr);
    matrix_.setBounds(botCols[1]);

    auto satA = meterCol.removeFromTop(meterCol.getHeight() / 2).reduced(4);
    satMeter_.setBounds(satA);
    auto grA = meterCol.reduced(4);
    grReadout_.setBounds(grA.removeFromBottom(18));
    grMeter_.setBounds(grA);
}

void HoldoverEditor::Content::updateMeters(float saturation, float gainReductionDb) {
    satMeter_.setLevel(saturation);
    grMeter_.setLevel(juce::jlimit(0.0f, 1.0f, gainReductionDb / 20.0f)); // 0..20 dB ladder
    grReadout_.setText(juce::String(gainReductionDb, 1) + " dB", juce::dontSendNotification);
}

// ============================ Editor ======================================

HoldoverEditor::HoldoverEditor(HoldoverProcessor& p)
    : AudioProcessorEditor(&p), processor(p), content_(p.apvts) {
    setLookAndFeel(&lnf_);
    addAndMakeVisible(content_);

    setResizable(true, true);
    // Constrain to the natural aspect ratio so uniform scaling never distorts.
    getConstrainer()->setFixedAspectRatio((double) kBaseWidth / (double) kBaseHeight);
    setResizeLimits(kBaseWidth / 2, kBaseHeight / 2, kBaseWidth * 2, kBaseHeight * 2);
    setSize(processor.uiWidth, processor.uiHeight);
    startTimerHz(30);
}

HoldoverEditor::~HoldoverEditor() { setLookAndFeel(nullptr); }

void HoldoverEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(HoldoverLookAndFeel::kBg));
}

void HoldoverEditor::resized() {
    // Lay the content out once at its natural size, then scale it uniformly to
    // fill the window. Aspect ratio is locked, so width and height scale together.
    content_.setBounds(0, 0, kBaseWidth, kBaseHeight);
    const float scale = (float) getWidth() / (float) kBaseWidth;
    content_.setTransform(juce::AffineTransform::scale(scale));

    processor.uiWidth = getWidth();
    processor.uiHeight = getHeight();
}

void HoldoverEditor::timerCallback() {
    const auto& m = processor.getMeters();
    content_.updateMeters(m.saturation(), m.gainReductionDb());
}

} // namespace holdover
