#include "LedMeter.h"

namespace holdover {

void LedMeter::paint(juce::Graphics& g) {
    auto area = getLocalBounds();
    auto capArea = area.removeFromBottom(64); // strip for the rotated caption

    const int lit = (int) std::round(level_ * kSegments);
    const float segH = (float) area.getHeight() / kSegments;
    for (int i = 0; i < kSegments; ++i) {
        auto seg = juce::Rectangle<float>((float) area.getX(),
            area.getBottom() - (i + 1) * segH + 1.0f,
            (float) area.getWidth(), segH - 2.0f);
        const float frac = (float) i / (kSegments - 1);
        juce::Colour on = frac < 0.66f ? juce::Colour(0xff5ad17a)
                        : frac < 0.85f ? juce::Colour(0xffd1c45a)
                                       : juce::Colour(0xffd15a5a);
        g.setColour(i < lit ? on : juce::Colour(0xff26262c));
        g.fillRoundedRectangle(seg, 2.0f);
    }

    // Caption rotated 90° so it reads bottom-to-top beside the narrow ladder.
    juce::Graphics::ScopedSaveState save(g);
    const auto c = capArea.getCentre().toFloat();
    g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, c.x, c.y));
    auto txt = juce::Rectangle<float>(0.0f, 0.0f, (float) capArea.getHeight(), (float) capArea.getWidth())
                   .withCentre(c);
    g.setColour(juce::Colour(0xff9a9aa2));
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText(caption_, txt, juce::Justification::centred);
}

} // namespace holdover
