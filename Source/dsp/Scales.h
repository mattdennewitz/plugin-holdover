#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace holdover::scales {

// Faders (INPUT, OUTPUT): position 0..10, unity (0 dB) at 5.
// 0..5 maps -60..0 dB, 5..10 maps 0..+12 dB. Position 0 is treated as silence.
inline float faderGain(float pos) noexcept {
    pos = juce::jlimit(0.0f, 10.0f, pos);
    if (pos <= 0.0f) return 0.0f;
    const float db = (pos <= 5.0f) ? juce::jmap(pos, 0.0f, 5.0f, -60.0f, 0.0f)
                                   : juce::jmap(pos, 5.0f, 10.0f, 0.0f, 12.0f);
    return juce::Decibels::decibelsToGain(db, -60.0f);
}

// Per-channel input trims (GAIN L/R): +/-24 dB, unity at 5, no silence floor.
inline float trimGain(float pos) noexcept {
    pos = juce::jlimit(0.0f, 10.0f, pos);
    const float db = juce::jmap(pos, 0.0f, 10.0f, -24.0f, 24.0f);
    return juce::Decibels::decibelsToGain(db);
}

// Matrix feed levels: silent at 0, unity at 10, log taper.
inline float feedGain(float pos) noexcept {
    pos = juce::jlimit(0.0f, 10.0f, pos);
    if (pos <= 0.0f) return 0.0f;
    const float db = juce::jmap(pos, 0.0f, 10.0f, -60.0f, 0.0f);
    return juce::Decibels::decibelsToGain(db, -60.0f);
}

// DRIVE pre-gain into the saturation cascade: unity at 5, -20..+20 dB across 0..10.
inline float drivePreGain(float pos) noexcept {
    pos = juce::jlimit(0.0f, 10.0f, pos);
    return juce::Decibels::decibelsToGain((pos - 5.0f) * 4.0f);
}

// MAKEUP gain: unity at 5, -20..+20 dB across 0..10. (Soft nonlinearity past unity
// is applied in the Compressor, not here.)
inline float makeupGain(float pos) noexcept {
    pos = juce::jlimit(0.0f, 10.0f, pos);
    return juce::Decibels::decibelsToGain((pos - 5.0f) * 4.0f);
}

} // namespace holdover::scales
