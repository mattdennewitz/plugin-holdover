#include "PluginEditor.h"
#include <array>
#include <cmath>

namespace holdover {

// ============================ Content =====================================

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

    grReadout_.setJustificationType(juce::Justification::centred);
    grReadout_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(grReadout_);
}

void HoldoverEditor::Content::paint(juce::Graphics& g) {
    // Wordmark: "HOLD" in text, "OVER" in accent.
    juce::Font f(juce::FontOptions(19.0f, juce::Font::bold));
    g.setFont(f);
    const float holdW = juce::GlyphArrangement::getStringWidth(f, "HOLD");
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
    {
        auto bar = headerArea_;
        bar.removeFromLeft(kWordmarkW);          // reserve space for the HOLD OVER wordmark
        presetBar_.setBounds(bar.withSizeKeepingCentre(bar.getWidth(), kChoiceBoxH));
    }
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

void HoldoverEditor::Content::updateMeters(float saturation, float gainReductionDb) {
    satMeter_.setLevel(saturation);
    grMeter_.setLevel(juce::jlimit(0.0f, 1.0f, gainReductionDb / 20.0f)); // 0..20 dB ladder
    grReadout_.setText(juce::String(gainReductionDb, 1) + " dB", juce::dontSendNotification);
}

void HoldoverEditor::Content::syncPresetBar() {
    presetBar_.syncSelection();
}

// ============================ Editor ======================================

HoldoverEditor::HoldoverEditor(HoldoverProcessor& p)
    : AudioProcessorEditor(&p), processor(p), content_(p) {
    setLookAndFeel(&lnf_);
    addAndMakeVisible(content_);

    // Snapshot the persisted size BEFORE touching the constrainer: setResizeLimits()
    // calls setBoundsConstrained(getBounds()) on the still-0x0 editor, which clamps it
    // up to the minimum and fires resized() — and resized() writes getWidth()/Height()
    // back into processor.uiWidth/uiHeight. Reading those as setSize() arguments after
    // that point would always yield the minimum, losing the restored size.
    const int minW = (int) std::round(kBaseWidth * 0.7);
    const int maxW = (int) std::round(kBaseWidth * 1.6);
    const int minH = (int) std::round(kBaseHeight * 0.7);
    const int maxH = (int) std::round(kBaseHeight * 1.6);

    // setSize() does NOT apply the constrainer, so a size loaded from an older session
    // could be out of range or off-aspect. Clamp width into the limits and derive the
    // height from the locked aspect ratio so the window never opens with a gap.
    const double aspect = (double) kBaseWidth / (double) kBaseHeight;
    const int restoreW = juce::jlimit(minW, maxW, processor.uiWidth);
    const int restoreH = (int) std::round(restoreW / aspect);

    setResizable(true, true);
    // Lock to the base aspect ratio so uniform scaling never distorts.
    getConstrainer()->setFixedAspectRatio(aspect);
    setResizeLimits(minW, minH, maxW, maxH);

    // Size from the snapshot; this final resized() restores processor.uiWidth/uiHeight.
    setSize(restoreW, restoreH);
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
    content_.syncPresetBar();
}

} // namespace holdover
