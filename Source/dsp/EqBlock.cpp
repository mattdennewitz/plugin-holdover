#include "EqBlock.h"
#include <juce_core/juce_core.h>
#include <cmath>

namespace holdover {

using juce::MathConstants;

EqBlock::Biquad EqBlock::peaking(float gainDb, float f, float q, double sr) noexcept {
    Biquad bq;
    const float A = std::pow(10.0f, gainDb / 40.0f);
    const float w0 = 2.0f * MathConstants<float>::pi * f / (float) sr;
    const float cw = std::cos(w0), sw = std::sin(w0);
    const float alpha = sw / (2.0f * q);
    const float a0 = 1.0f + alpha / A;
    bq.b0 = (1.0f + alpha * A) / a0;
    bq.b1 = (-2.0f * cw) / a0;
    bq.b2 = (1.0f - alpha * A) / a0;
    bq.a1 = (-2.0f * cw) / a0;
    bq.a2 = (1.0f - alpha / A) / a0;
    return bq;
}

EqBlock::Biquad EqBlock::lowShelf(float gainDb, float f, double sr) noexcept {
    Biquad bq;
    const float A = std::pow(10.0f, gainDb / 40.0f);
    const float w0 = 2.0f * MathConstants<float>::pi * f / (float) sr;
    const float cw = std::cos(w0), sw = std::sin(w0);
    const float alpha = sw / 2.0f * std::sqrt(2.0f);
    const float tsa = 2.0f * std::sqrt(A) * alpha;
    const float a0 = (A + 1.0f) + (A - 1.0f) * cw + tsa;
    bq.b0 = (A * ((A + 1.0f) - (A - 1.0f) * cw + tsa)) / a0;
    bq.b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cw)) / a0;
    bq.b2 = (A * ((A + 1.0f) - (A - 1.0f) * cw - tsa)) / a0;
    bq.a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cw)) / a0;
    bq.a2 = ((A + 1.0f) + (A - 1.0f) * cw - tsa) / a0;
    return bq;
}

EqBlock::Biquad EqBlock::highShelf(float gainDb, float f, double sr) noexcept {
    Biquad bq;
    const float A = std::pow(10.0f, gainDb / 40.0f);
    const float w0 = 2.0f * MathConstants<float>::pi * f / (float) sr;
    const float cw = std::cos(w0), sw = std::sin(w0);
    const float alpha = sw / 2.0f * std::sqrt(2.0f);
    const float tsa = 2.0f * std::sqrt(A) * alpha;
    const float a0 = (A + 1.0f) - (A - 1.0f) * cw + tsa;
    bq.b0 = (A * ((A + 1.0f) + (A - 1.0f) * cw + tsa)) / a0;
    bq.b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cw)) / a0;
    bq.b2 = (A * ((A + 1.0f) + (A - 1.0f) * cw - tsa)) / a0;
    bq.a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cw)) / a0;
    bq.a2 = ((A + 1.0f) - (A - 1.0f) * cw - tsa) / a0;
    return bq;
}

// Proportional Q: |gain| 0 -> Q 0.7 (broad), 12 dB -> Q 3.0 (tight).
float EqBlock::presenceQ(float gainDb) noexcept {
    const float mag = juce::jlimit(0.0f, 12.0f, std::abs(gainDb));
    return juce::jmap(mag, 0.0f, 12.0f, 0.7f, 3.0f);
}

void EqBlock::prepare(double sampleRate) noexcept { sr_ = sampleRate; reset(); }
void EqBlock::reset() noexcept { low_.reset(); mid_.reset(); high_.reset(); }

// Setters copy only coefficient fields, preserving z1/z2 state for realtime safety.
void EqBlock::setBass(float gainDb, float f) noexcept {
    auto c = lowShelf(gainDb, f, sr_);
    low_.b0 = c.b0; low_.b1 = c.b1; low_.b2 = c.b2; low_.a1 = c.a1; low_.a2 = c.a2;
}

void EqBlock::setTreble(float gainDb, float f) noexcept {
    auto c = highShelf(gainDb, f, sr_);
    high_.b0 = c.b0; high_.b1 = c.b1; high_.b2 = c.b2; high_.a1 = c.a1; high_.a2 = c.a2;
}

void EqBlock::setPresence(float gainDb, float f) noexcept {
    auto c = peaking(gainDb, f, presenceQ(gainDb), sr_);
    mid_.b0 = c.b0; mid_.b1 = c.b1; mid_.b2 = c.b2; mid_.a1 = c.a1; mid_.a2 = c.a2;
}

float EqBlock::processSample(float x) noexcept {
    return high_.process(mid_.process(low_.process(x)));
}

} // namespace holdover
