#include "Parameters.h"

namespace holdover::params {

using APF  = juce::AudioParameterFloat;
using APC  = juce::AudioParameterChoice;
using APB  = juce::AudioParameterBool;
using Range = juce::NormalisableRange<float>;

// Log/skewed frequency range helper.
static Range freqRange(float lo, float hi) {
    Range r(lo, hi, 0.0f);
    r.setSkewForCentre(std::sqrt(lo * hi));
    return r;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto pid = [](const char* id) { return juce::ParameterID { id, 1 }; };

    // --- Continuous (0-10 position knobs) ---
    p.push_back(std::make_unique<APF>(pid("gainL"),  "Gain L",  Range(0.0f, 10.0f, 0.01f), 5.0f));
    p.push_back(std::make_unique<APF>(pid("gainR"),  "Gain R",  Range(0.0f, 10.0f, 0.01f), 5.0f));
    p.push_back(std::make_unique<APF>(pid("input"),  "Input",   Range(0.0f, 10.0f, 0.01f), 5.0f));

    // --- Filters ---
    p.push_back(std::make_unique<APF>(pid("hpfFreq"), "HPF Freq", freqRange(20.0f, 4700.0f), 20.0f));
    p.push_back(std::make_unique<APF>(pid("hpfPeak"), "HPF Peak", Range(0.0f, 10.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<APF>(pid("lpfFreq"), "LPF Freq", freqRange(220.0f, 22000.0f), 22000.0f));
    p.push_back(std::make_unique<APF>(pid("lpfPeak"), "LPF Peak", Range(0.0f, 10.0f, 0.01f), 0.0f));

    // --- EQ ---
    p.push_back(std::make_unique<APF>(pid("presenceDb"),   "Presence",      Range(-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<APF>(pid("presenceFreq"), "Presence Freq", freqRange(260.0f, 12000.0f), 2000.0f));
    p.push_back(std::make_unique<APF>(pid("bassDb"),       "Bass",          Range(-18.0f, 18.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<APF>(pid("trebleDb"),     "Treble",        Range(-15.0f, 15.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<APC>(pid("bassFreq"),   "Bass Freq",   bassFreqOptions, 1));
    p.push_back(std::make_unique<APC>(pid("trebleFreq"), "Treble Freq", trebleFreqOptions, 1));

    // --- Compressor ---
    p.push_back(std::make_unique<APF>(pid("threshold"), "Threshold", Range(-40.0f, 0.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<APF>(pid("behavior"),  "Behavior",  Range(0.0f, 10.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<APF>(pid("makeup"),    "Makeup",    Range(0.0f, 10.0f, 0.01f), 5.0f));
    p.push_back(std::make_unique<APC>(pid("attack"),  "Attack",  timeOptions, 1));
    p.push_back(std::make_unique<APC>(pid("release"), "Release", timeOptions, 1));
    p.push_back(std::make_unique<APC>(pid("scSource"), "SC Source", scSourceOptions, 2));

    // --- Drive ---
    p.push_back(std::make_unique<APF>(pid("drive"), "Drive", Range(0.0f, 10.0f, 0.01f), 5.0f));
    p.push_back(std::make_unique<APC>(pid("masMode"), "MAS", masOptions, 0));
    p.push_back(std::make_unique<APF>(pid("character"), "Character", Range(0.0f, 10.0f, 0.01f), 0.0f));

    // --- Matrix / Output ---
    p.push_back(std::make_unique<APF>(pid("dryEqFeedLevel"), "Dry/EQ Feed", Range(0.0f, 10.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<APF>(pid("compFeedLevel"),  "Comp Feed",   Range(0.0f, 10.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<APF>(pid("satFeedLevel"),   "Sat Feed",    Range(0.0f, 10.0f, 0.01f), 10.0f));
    p.push_back(std::make_unique<APF>(pid("output"), "Output", Range(0.0f, 10.0f, 0.01f), 5.0f));
    p.push_back(std::make_unique<APC>(pid("dryEqSource"), "Dry/EQ Source", dryEqSrcOptions, 1));

    // --- Toggles ---
    p.push_back(std::make_unique<APB>(pid("filtEngage"), "Filter In", false));
    p.push_back(std::make_unique<APB>(pid("eqEngage"),   "EQ In",     true));
    p.push_back(std::make_unique<APB>(pid("compEngage"), "Comp In",   true));
    p.push_back(std::make_unique<APB>(pid("curve"),      "Curve",     true));
    p.push_back(std::make_unique<APB>(pid("rmsMode"),    "RMS",       false));
    p.push_back(std::make_unique<APB>(pid("scFilter"),   "SC Filter", false));
    p.push_back(std::make_unique<APB>(pid("satEngage"),  "SAT",       false));
    p.push_back(std::make_unique<APB>(pid("hexEngage"),  "HEX",       false));
    p.push_back(std::make_unique<APB>(pid("dryEqMute"),  "Dry/EQ Mute", true));
    p.push_back(std::make_unique<APB>(pid("compMute"),   "Comp Mute",   true));
    p.push_back(std::make_unique<APB>(pid("satMute"),    "Sat Mute",    false));
    p.push_back(std::make_unique<APB>(pid("ceiling"),    "Ceiling",     false));

    return { p.begin(), p.end() };
}

const juce::StringArray& allIDs() {
    static const juce::StringArray ids {
        "gainL","gainR","input","hpfFreq","hpfPeak","lpfFreq","lpfPeak",
        "presenceDb","presenceFreq","bassDb","trebleDb","threshold","behavior",
        "makeup","drive","dryEqFeedLevel","compFeedLevel","satFeedLevel","output",
        "bassFreq","trebleFreq","attack","release","masMode","scSource","dryEqSource",
        "filtEngage","eqEngage","compEngage","curve","rmsMode","scFilter",
        "satEngage","hexEngage","dryEqMute","compMute","satMute","ceiling","character" };
    return ids;
}

} // namespace holdover::params
