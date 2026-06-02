#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace holdover {

using APVTS = juce::AudioProcessorValueTreeState;

// A labelled rotary knob bound to an APVTS parameter. Owns its attachment.
struct AttachedKnob {
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag,
                          juce::Slider::TextBoxBelow };
    juce::Label  label;
    std::unique_ptr<APVTS::SliderAttachment> attachment;

    void attach(juce::Component& parent, APVTS& s, const juce::String& id, const juce::String& text) {
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
        parent.addAndMakeVisible(slider);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(11.0f)));
        parent.addAndMakeVisible(label);
        attachment = std::make_unique<APVTS::SliderAttachment>(s, id, slider);
    }
    // Lay out within `area`: label on top, knob below.
    void layout(juce::Rectangle<int> area) {
        label.setBounds(area.removeFromTop(14));
        slider.setBounds(area);
    }
};

struct AttachedToggle {
    juce::ToggleButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
    void attach(juce::Component& parent, APVTS& s, const juce::String& id, const juce::String& text) {
        button.setButtonText(text);
        parent.addAndMakeVisible(button);
        attachment = std::make_unique<APVTS::ButtonAttachment>(s, id, button);
    }
    void layout(juce::Rectangle<int> area) { button.setBounds(area); }
};

struct AttachedChoice {
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
    void attach(juce::Component& parent, APVTS& s, const juce::String& id,
                const juce::String& text, const juce::StringArray& items) {
        box.addItemList(items, 1);
        parent.addAndMakeVisible(box);
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(juce::FontOptions(11.0f)));
        parent.addAndMakeVisible(label);
        attachment = std::make_unique<APVTS::ComboBoxAttachment>(s, id, box);
    }
    void layout(juce::Rectangle<int> area) {
        label.setBounds(area.removeFromTop(14));
        box.setBounds(area.removeFromTop(22));
    }
};

} // namespace holdover
