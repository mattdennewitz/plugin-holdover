#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Layout.h"

namespace holdover {

using APVTS = juce::AudioProcessorValueTreeState;

// How a toggle is drawn. Carried on the button via a property so the look-and-feel
// can pick the renderer without subclassing ToggleButton.
enum class ToggleStyle { Switch, Led, Mute };

inline void tagToggleStyle(juce::ToggleButton& b, ToggleStyle s) {
    b.getProperties().set("hldStyle", (int) s);
}

// A labelled rotary knob bound to an APVTS parameter. Owns its attachment.
struct AttachedKnob {
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag,
                          juce::Slider::TextBoxBelow };
    juce::Label  label;
    std::unique_ptr<APVTS::SliderAttachment> attachment;

    void attach(juce::Component& parent, APVTS& s, const juce::String& id, const juce::String& text) {
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, ui::kCellW - 8, ui::kValueH);
        parent.addAndMakeVisible(slider);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(10.0f)));
        parent.addAndMakeVisible(label);
        attachment = std::make_unique<APVTS::SliderAttachment>(s, id, slider);
    }
    // Label on top (kLabelH); the slider fills the rest and draws the rotary above its
    // own TextBoxBelow, so the rotary lands at ~kKnobH across every panel.
    void layout(juce::Rectangle<int> area) {
        label.setBounds(area.removeFromTop(ui::kLabelH));
        slider.setBounds(area);
    }
};

struct AttachedToggle {
    juce::ToggleButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
    void attach(juce::Component& parent, APVTS& s, const juce::String& id,
                const juce::String& text, ToggleStyle style) {
        button.setButtonText(text);
        tagToggleStyle(button, style);
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
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(10.0f)));
        parent.addAndMakeVisible(label);
        attachment = std::make_unique<APVTS::ComboBoxAttachment>(s, id, box);
    }
    // Label on top (kLabelH); the dropdown box is vertically centred in the remaining
    // space so it lines up with the rotaries in the same row.
    void layout(juce::Rectangle<int> area) {
        label.setBounds(area.removeFromTop(ui::kLabelH));
        box.setBounds(area.withSizeKeepingCentre(area.getWidth(), ui::kChoiceBoxH));
    }
};

} // namespace holdover
