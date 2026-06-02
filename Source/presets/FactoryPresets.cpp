#include "FactoryPresets.h"

namespace holdover::presets {

const std::vector<Preset>& all() {
    static const std::vector<Preset> bank = {
        { "Init / Flat", {
            {"input",5},{"output",5},{"drive",5},{"makeup",5},{"threshold",0},
            {"behavior",0},{"satFeedLevel",10},{"satMute",0},{"dryEqMute",1},{"compMute",1},
            {"eqEngage",1},{"compEngage",1},{"filtEngage",0} } },

        { "Vocal Glue", {
            {"compEngage",1},{"threshold",-18},{"behavior",4},{"attack",1},{"release",1},
            {"makeup",6},{"masMode",1},{"drive",6},{"satFeedLevel",10},{"compFeedLevel",6},
            {"compMute",0},{"scFilter",1},{"presenceDb",3},{"presenceFreq",4000} } },

        { "Drum Bus Bounce", {
            {"compEngage",1},{"threshold",-22},{"behavior",8},{"attack",0},{"release",0},
            {"makeup",7},{"drive",7},{"satEngage",1},{"satFeedLevel",10},{"compFeedLevel",7},
            {"compMute",0},{"bassDb",4},{"bassFreq",1} } },

        { "Bass Saturator", {
            {"drive",8},{"masMode",2},{"satEngage",1},{"curve",1},{"satFeedLevel",10},
            {"threshold",-12},{"behavior",3},{"compEngage",1},{"makeup",6} } },

        { "Mix Bus Ceiling", {
            {"compEngage",1},{"threshold",-16},{"behavior",2},{"attack",2},{"release",1},
            {"drive",6},{"masMode",1},{"satFeedLevel",10},{"ceiling",1},{"output",5} } },

        { "Aggressive HEX", {
            {"drive",9},{"hexEngage",1},{"satEngage",1},{"curve",0},{"satFeedLevel",10},
            {"ceiling",1},{"threshold",-20},{"behavior",6},{"compEngage",1} } },

        { "Parallel Crush", {
            {"compEngage",1},{"threshold",-30},{"behavior",10},{"attack",0},{"release",0},
            {"makeup",8},{"compFeedLevel",8},{"compMute",0},{"satFeedLevel",7},
            {"drive",6},{"satEngage",1} } },

        { "Filter Sweep", {
            {"filtEngage",1},{"lpfFreq",1200},{"lpfPeak",7},{"hpfFreq",120},{"hpfPeak",4},
            {"eqEngage",1},{"satFeedLevel",10},{"drive",5} } },
    };
    return bank;
}

void apply(juce::AudioProcessorValueTreeState& apvts, int index) {
    if (index < 0 || index >= (int) all().size()) return;
    for (const auto& [id, value] : all()[(size_t) index].values)
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(value));
}

} // namespace holdover::presets
