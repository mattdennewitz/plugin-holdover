#include "HoldoverLookAndFeel.h"
#include "ControlHelpers.h"  // for ToggleStyle

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
    const bool on = b.getToggleState();
    const auto style = (ToggleStyle) (int) b.getProperties().getWithDefault("hldStyle", (int) ToggleStyle::Led);
    auto bounds = b.getLocalBounds().toFloat().reduced(1.0f);

    if (style == ToggleStyle::Switch) {
        // Sliding pill on the left, bold label to its right.
        const float trackW = 38.0f, trackH = 20.0f;
        auto track = juce::Rectangle<float>(trackW, trackH)
                         .withY(bounds.getCentreY() - trackH * 0.5f).withX(bounds.getX());
        g.setColour(juce::Colour(on ? kAccent : kTrack));
        g.fillRoundedRectangle(track, trackH * 0.5f);
        const float thumbD = trackH - 4.0f;
        const float thumbX = on ? track.getRight() - thumbD - 2.0f : track.getX() + 2.0f;
        g.setColour(juce::Colour(on ? 0xff10100f : 0xff6a6a72));
        g.fillEllipse(thumbX, track.getY() + 2.0f, thumbD, thumbD);
        g.setColour(juce::Colour(on ? kText : kTextDim));
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(b.getButtonText(),
                   bounds.withTrimmedLeft(trackW + 8.0f), juce::Justification::centredLeft);
        if (highlighted) {
            g.setColour(juce::Colour(0x18ffffff));
            g.fillRoundedRectangle(track, trackH * 0.5f);
        }
        return;
    }

    // LED button (and mute variant): bordered button, label, glowing indicator dot.
    const juce::uint32 accent = (style == ToggleStyle::Mute) ? kMute : kAccent;
    g.setColour(juce::Colour(on ? (style == ToggleStyle::Mute ? 0xff221416u : 0xff15201fu) : 0xff1a1a1eu));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(on ? accent : 0xff2a2a30));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    if (highlighted) { g.setColour(juce::Colour(0x18ffffff)); g.fillRoundedRectangle(bounds, 4.0f); }

    const float dotD = 7.0f;
    auto dot = juce::Rectangle<float>(dotD, dotD)
                   .withY(bounds.getCentreY() - dotD * 0.5f)
                   .withX(bounds.getRight() - dotD - 8.0f);
    if (on) { g.setColour(juce::Colour(accent).withAlpha(0.35f)); g.fillEllipse(dot.expanded(2.5f)); }
    g.setColour(juce::Colour(on ? accent : kOffDot));
    g.fillEllipse(dot);

    g.setColour(juce::Colour(on ? kText : kTextDim));
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.drawText(b.getButtonText(),
               b.getLocalBounds().withTrimmedRight(18).withTrimmedLeft(8),
               juce::Justification::centredLeft);
}

} // namespace holdover
