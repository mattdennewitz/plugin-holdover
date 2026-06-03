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
            {"makeup",6},{"masMode",1},{"drive",6},{"character",6},{"satFeedLevel",10},{"compFeedLevel",6},
            {"compMute",0},{"scFilter",1},{"presenceDb",3},{"presenceFreq",4000} } },

        { "Drum Bus Bounce", {
            {"compEngage",1},{"threshold",-22},{"behavior",8},{"attack",0},{"release",0},
            {"makeup",7},{"drive",7},{"satEngage",1},{"satFeedLevel",10},{"compFeedLevel",7},
            {"compMute",0},{"bassDb",4},{"bassFreq",1} } },

        { "Bass Saturator", {
            {"drive",8},{"masMode",2},{"satEngage",1},{"curve",1},{"satFeedLevel",10},
            {"threshold",-12},{"behavior",3},{"compEngage",1},{"makeup",6},{"character",7} } },

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

        // Built to feature the Character macro at its most audible: moderate drive + MAS-2nd
        // (where the Class-A bias bites) feeding heavy, fast compression (where the VCA THD
        // blooms). Load it, then sweep Character 10 -> 0 to hear the analog coloration come
        // and go. No SAT/HEX so the even-harmonic warmth isn't masked by hard clipping.
        { "Character Showcase", {
            {"compEngage",1},{"threshold",-26},{"behavior",9},{"attack",0},{"release",0},
            {"makeup",7},{"drive",6},{"masMode",1},{"curve",1},{"character",10},
            {"satEngage",0},{"hexEngage",0},{"eqEngage",0},{"filtEngage",0},
            {"satFeedLevel",10},{"satMute",0},{"dryEqMute",1},{"compMute",1},{"output",5} } },

        // Isolates Character's Drive-bias mechanism: the compressor is OFF and SAT/HEX
        // are off, so the only thing coloring the audible (sat-feed = drive-output) path
        // is the Character-biased MAS-2nd shaper. Sweep Character 0 -> 8 to hear pure
        // even-harmonic "Class-A" warmth bloom. Useful on a DI, synth, or mix bus.
        { "Console Warmth", {
            {"compEngage",0},{"drive",5},{"masMode",1},{"curve",1},{"character",8},
            {"satEngage",0},{"hexEngage",0},{"eqEngage",0},{"filtEngage",0},
            {"satFeedLevel",10},{"satMute",0},{"compMute",1},{"dryEqMute",1},{"output",5} } },

        // Isolates Character's Comp-VCA-THD mechanism: masMode is OFF (Drive is identity)
        // and SAT/HEX are off, so the compressor is the only harmonic source. Makeup sits at
        // unity (no makeup saturation) and Output restores level, so Character is the ONLY
        // harmonic variable: the comp feed carries grDrive = Character x gainReduction. Sweep
        // Character 0 -> 8 to hear clean glue turn into gritty "British bus" bite that tracks
        // how hard it clamps.
        { "Bus Glue Grit", {
            {"compEngage",1},{"threshold",-22},{"behavior",5},{"attack",1},{"release",2},
            {"makeup",5},{"masMode",0},{"satEngage",0},{"hexEngage",0},{"character",8},
            {"eqEngage",0},{"filtEngage",0},
            {"compFeedLevel",10},{"compMute",0},{"satMute",1},{"dryEqMute",1},{"output",8} } },
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
