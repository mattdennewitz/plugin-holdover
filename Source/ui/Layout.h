#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace holdover::ui {

// One source of truth for editor layout. Every knob/choice cell is identical, so
// controls line up vertically across panels and both columns align.
constexpr int kCellW        = 62;   // width of one knob/choice cell
constexpr int kCellGap      = 8;    // horizontal gap between cells in a row
constexpr int kLabelH       = 14;   // label strip above a control
constexpr int kKnobH        = 64;   // vertical space for the rotary (drawn ø ~56 after the L&F inset)
constexpr int kValueH       = 16;   // value readout strip below a knob
constexpr int kCellH        = kLabelH + kKnobH + kValueH; // full knob-cell height (86)
constexpr int kChoiceBoxH   = 22;   // dropdown box height
constexpr int kToggleH      = 24;   // switch / LED-button row height
constexpr int kRowGap       = 10;   // vertical gap between rows / panels / regions
constexpr int kPanelPadX    = 12;   // panel horizontal padding
constexpr int kPanelPadTop  = 8;    // gap below a panel title before content
constexpr int kPanelPadBot  = 10;   // panel bottom padding
constexpr int kPanelTitleH  = 22;   // panel title bar height
constexpr int kHeaderH      = 36;   // top wordmark band
constexpr int kBridgeW      = 96;   // meter bridge width
constexpr int kWordmarkW    = 160;  // left reserve in the header for the HOLD OVER wordmark

// Lay out `n` fixed-width (kCellW) cells left-to-right in `area`, separated by
// kCellGap. Fixed width (not flex) is what makes a knob in INPUT identical to one in
// EQ, and makes their columns line up vertically.
inline std::vector<juce::Rectangle<int>> cells(juce::Rectangle<int> area, int n) {
    std::vector<juce::Rectangle<int>> out;
    out.reserve((size_t) n);
    for (int i = 0; i < n; ++i) {
        out.push_back(area.removeFromLeft(kCellW));
        if (i < n - 1) area.removeFromLeft(kCellGap);
    }
    return out;
}

} // namespace holdover::ui
