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
    const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(6.0f);
    const auto centre = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float angle = a0 + pos * (a1 - a0);

    g.setColour(juce::Colour(0xff2a2a30));
    g.drawEllipse(bounds, 2.0f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, a0, angle, true);
    g.setColour(juce::Colour(kAccent));
    g.strokePath(arc, juce::PathStrokeType(2.5f));

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
