#include <catch2/catch_test_macros.hpp>
#include "dsp/ChannelStrip.h"
#include <cmath>

namespace {
constexpr double kSr = 48000.0;
constexpr int kBlock = 512;

holdover::ChannelStrip::Targets neutralTargets() {
    holdover::ChannelStrip::Targets t {};
    t.gainLpos = t.gainRpos = t.inputPos = 5.0f;
    t.hpfFreq = 20.0f; t.lpfFreq = 22000.0f;
    t.presenceFreq = 2000.0f; t.bassFreq = 150.0f; t.trebleFreq = 10000.0f;
    t.thresholdDb = 0.0f; t.behaviorPos = 0.0f; t.makeupPos = 5.0f;
    t.attackIdx = 1; t.releaseIdx = 1;
    t.drivePos = 5.0f; t.masMode = 0;
    t.dryEqFeedPos = 0.0f; t.compFeedPos = 0.0f; t.satFeedPos = 10.0f;
    t.outputPos = 5.0f; t.scSource = 2; t.dryEqSource = 1;
    t.filtEngage = false; t.eqEngage = false; t.compEngage = false;
    t.curve = true; t.rmsMode = false; t.scFilter = false;
    t.satEngage = false; t.hexEngage = false;
    t.dryEqMute = true; t.compMute = true; t.satMute = false;
    t.ceiling = false;
    return t;
}

float runRms(holdover::ChannelStrip& s, float freq, float amp, int blocks = 40) {
    const double w = 2.0 * juce::MathConstants<double>::pi * freq / kSr;
    int phase = 0; double acc = 0; int counted = 0;
    juce::AudioBuffer<float> main(2, kBlock), sc(2, kBlock);
    for (int b = 0; b < blocks; ++b) {
        for (int n = 0; n < kBlock; ++n, ++phase) {
            const float v = amp * (float) std::sin(w * phase);
            main.setSample(0, n, v); main.setSample(1, n, v);
        }
        sc.clear();
        s.processBlock(main, &sc);
        if (b >= blocks / 2)
            for (int n = 0; n < kBlock; ++n) { const float y = main.getSample(0, n); acc += (double) y * y; ++counted; }
    }
    return (float) std::sqrt(acc / counted);
}
}

TEST_CASE("default all-off routing is near unity passthrough", "[strip]") {
    holdover::ChannelStrip s; s.prepare(kSr, kBlock, 2);
    s.setTargets(neutralTargets());
    const float outRms = runRms(s, 1000.0f, 0.25f);
    const float inRms = 0.25f / std::sqrt(2.0f);
    REQUIRE(juce::Decibels::gainToDecibels(outRms / inRms) > -0.7f);
    REQUIRE(juce::Decibels::gainToDecibels(outRms / inRms) <  0.7f);
}

TEST_CASE("reports nonzero latency from oversampling", "[strip]") {
    holdover::ChannelStrip s; s.prepare(kSr, kBlock, 2);
    REQUIRE(s.getLatencySamples() > 0);
}

TEST_CASE("disengaged EQ feeds only its parallel feed, not the serial path", "[strip]") {
    holdover::ChannelStrip s; s.prepare(kSr, kBlock, 2);
    auto t = neutralTargets();
    t.eqEngage = false; t.bassDb = 18.0f; t.bassFreq = 150.0f;
    s.setTargets(t);
    const float serialOnly = runRms(s, 50.0f, 0.1f);

    auto t2 = t; t2.dryEqMute = false; t2.dryEqSource = 1; t2.dryEqFeedPos = 10.0f;
    t2.satMute = true;
    holdover::ChannelStrip s2; s2.prepare(kSr, kBlock, 2); s2.setTargets(t2);
    const float dryEqPost = runRms(s2, 50.0f, 0.1f);

    REQUIRE(dryEqPost > serialOnly * 1.5f);
}

TEST_CASE("no NaNs under extreme drive", "[strip]") {
    holdover::ChannelStrip s; s.prepare(kSr, kBlock, 2);
    auto t = neutralTargets();
    t.drivePos = 10.0f; t.masMode = 2; t.satEngage = true; t.hexEngage = true;
    t.ceiling = true;
    s.setTargets(t);
    juce::AudioBuffer<float> main(2, kBlock), sc(2, kBlock);
    for (int b = 0; b < 20; ++b) {
        for (int n = 0; n < kBlock; ++n) { main.setSample(0, n, 0.9f); main.setSample(1, n, -0.9f); }
        sc.clear(); s.processBlock(main, &sc);
        for (int n = 0; n < kBlock; ++n) REQUIRE(std::isfinite(main.getSample(0, n)));
    }
}

TEST_CASE("oversampling rejects aliasing under hard clipping", "[strip]") {
    holdover::ChannelStrip s; s.prepare(kSr, kBlock, 2);
    auto t = neutralTargets();
    t.drivePos = 9.0f; t.hexEngage = true;
    s.setTargets(t);
    const double w = 2.0 * juce::MathConstants<double>::pi * 7000.0 / kSr;
    juce::AudioBuffer<float> main(2, kBlock), sc(2, kBlock);
    constexpr int order = 14, size = 1 << order;
    std::vector<float> fftbuf(size * 2, 0.0f);
    int phase = 0, filled = 0;
    while (filled < size) {
        for (int n = 0; n < kBlock; ++n, ++phase) { float v = 0.5f*(float)std::sin(w*phase); main.setSample(0,n,v); main.setSample(1,n,v); }
        sc.clear(); s.processBlock(main, &sc);
        for (int n = 0; n < kBlock && filled < size; ++n, ++filled) fftbuf[filled] = main.getSample(0, n);
    }
    juce::dsp::FFT fft(order);
    juce::dsp::WindowingFunction<float> win(size, juce::dsp::WindowingFunction<float>::hann);
    win.multiplyWithWindowingTable(fftbuf.data(), size);
    fft.performFrequencyOnlyForwardTransform(fftbuf.data());
    auto binAt = [&](float hz){ return fftbuf[(int) std::round(hz / kSr * size)]; };
    REQUIRE(binAt(13000.0f) < binAt(7000.0f) * 0.05f);
}
