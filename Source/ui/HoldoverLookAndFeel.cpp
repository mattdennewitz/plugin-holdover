#include "HoldoverLookAndFeel.h"

namespace holdover {

HoldoverLookAndFeel::HoldoverLookAndFeel() {
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(kText));
    setColour(juce::Label::textColourId, juce::Colour(kText));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(kPanel));
    setColour(juce::ComboBox::textColourId, juce::Colour(kText));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff34343a));
}

void HoldoverLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                           float pos, float a0, float a1, juce::Slider&) {
    // Always draw inside the largest centred SQUARE of the slot so the knob is a
    // true circle regardless of the slot's aspect ratio. The border ring and the
    // value arc share one radius, so controls and borders line up exactly.
    const auto slot = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(6.0f);
    const float diameter = juce::jmin(slot.getWidth(), slot.getHeight());
    const auto square = juce::Rectangle<float>(diameter, diameter).withCentre(slot.getCentre());
    const auto centre = square.getCentre();
    const float angle = a0 + pos * (a1 - a0);

    constexpr float lineW = 2.5f;
    const float radius = diameter * 0.5f - lineW * 0.5f; // arc centreline = border centreline

    // Border ring (full circle) — its stroke centreline sits at `radius`.
    g.setColour(juce::Colour(0xff2a2a30));
    g.drawEllipse(square.reduced(lineW * 0.5f), lineW);

    // Value arc on the same circle as the border.
    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, a0, angle, true);
    g.setColour(juce::Colour(kAccent));
    g.strokePath(arc, juce::PathStrokeType(lineW));

    // Pointer from centre to the ring.
    juce::Path pointer;
    pointer.startNewSubPath(centre.x, centre.y);
    pointer.lineTo(centre.x + radius * std::sin(angle), centre.y - radius * std::cos(angle));
    g.setColour(juce::Colour(kText));
    g.strokePath(pointer, juce::PathStrokeType(2.0f));
}

void HoldoverLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b,
                                           bool highlighted, bool) {
    auto bounds = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = b.getToggleState();
    g.setColour(juce::Colour(on ? kAccent : 0xff26262c));
    g.fillRoundedRectangle(bounds, 3.0f);
    if (highlighted) { g.setColour(juce::Colour(0x22ffffff)); g.fillRoundedRectangle(bounds, 3.0f); }
    g.setColour(juce::Colour(on ? 0xff10100f : kText));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

} // namespace holdover
