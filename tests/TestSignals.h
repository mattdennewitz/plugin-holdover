#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

namespace test {

// Process a sine of `freq` Hz through `fn` (float->float) and return RMS of the
// steady-state tail (skips the first quarter to avoid transient).
template <typename Fn>
inline float steadyStateRms(Fn&& fn, double sampleRate, float freq, int numSamples = 16384) {
    const double w = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
    double acc = 0.0; int counted = 0;
    for (int n = 0; n < numSamples; ++n) {
        const float x = (float) std::sin(w * n);
        const float y = fn(x);
        if (n >= numSamples / 4) { acc += (double) y * y; ++counted; }
    }
    return (float) std::sqrt(acc / juce::jmax(1, counted));
}

// Magnitude (dB) of a filter at `freq`, relative to a 1.0-amplitude sine input.
template <typename Fn>
inline float magnitudeDb(Fn&& fn, double sampleRate, float freq) {
    const float rms = steadyStateRms(std::forward<Fn>(fn), sampleRate, freq);
    const float inputRms = 1.0f / std::sqrt(2.0f); // RMS of unit-amplitude sine
    return juce::Decibels::gainToDecibels(rms / inputRms);
}

// Harmonic magnitudes via FFT: returns linear magnitude at each integer harmonic
// of `fundamental`. Index 0 = fundamental, 1 = 2nd harmonic, etc.
template <typename Fn>
inline std::vector<float> harmonicMagnitudes(Fn&& fn, double sampleRate,
                                             float fundamental, int numHarmonics) {
    constexpr int order = 14;                 // 16384-point FFT
    constexpr int size  = 1 << order;
    juce::dsp::FFT fft(order);
    std::vector<float> buf(size * 2, 0.0f);
    juce::dsp::WindowingFunction<float> win(size, juce::dsp::WindowingFunction<float>::hann);
    const double w = 2.0 * juce::MathConstants<double>::pi * fundamental / sampleRate;
    for (int n = 0; n < size; ++n) buf[n] = fn((float) std::sin(w * n));
    win.multiplyWithWindowingTable(buf.data(), size);
    fft.performFrequencyOnlyForwardTransform(buf.data());
    std::vector<float> out;
    for (int h = 1; h <= numHarmonics; ++h) {
        const int bin = (int) std::round(fundamental * h / sampleRate * size);
        out.push_back(bin < size / 2 ? buf[bin] : 0.0f);
    }
    return out;
}

} // namespace test
