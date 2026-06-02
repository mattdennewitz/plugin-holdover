#include <catch2/catch_test_macros.hpp>
#include "dsp/ChannelStrip.h"
#include <cmath>

namespace { constexpr double kSr = 44100.0; constexpr int kBlock = 256; }

TEST_CASE("randomized parameter sweeps never produce NaN/Inf or runaway", "[hardening]") {
    holdover::ChannelStrip s; s.prepare(kSr, kBlock, 2);
    juce::Random rng (0x5eed); // fixed seed: deterministic
    juce::AudioBuffer<float> main(2, kBlock), sc(2, kBlock);

    for (int iter = 0; iter < 200; ++iter) {
        holdover::ChannelStrip::Targets t {};
        t.gainLpos = rng.nextFloat()*10; t.gainRpos = rng.nextFloat()*10; t.inputPos = rng.nextFloat()*10;
        t.hpfFreq = 20 + rng.nextFloat()*4680; t.hpfPeak = rng.nextFloat()*10;
        t.lpfFreq = 220 + rng.nextFloat()*21780; t.lpfPeak = rng.nextFloat()*10;
        t.presenceDb = rng.nextFloat()*24-12; t.presenceFreq = 260 + rng.nextFloat()*11740;
        t.bassDb = rng.nextFloat()*36-18; t.bassFreq = 150; t.trebleDb = rng.nextFloat()*30-15; t.trebleFreq = 10000;
        t.thresholdDb = -rng.nextFloat()*40; t.behaviorPos = rng.nextFloat()*10; t.makeupPos = rng.nextFloat()*10;
        t.attackIdx = rng.nextInt(3); t.releaseIdx = rng.nextInt(3);
        t.drivePos = rng.nextFloat()*10; t.masMode = rng.nextInt(3);
        t.dryEqFeedPos = rng.nextFloat()*10; t.compFeedPos = rng.nextFloat()*10; t.satFeedPos = rng.nextFloat()*10;
        t.outputPos = rng.nextFloat()*10; t.scSource = rng.nextInt(3); t.dryEqSource = rng.nextInt(2);
        t.filtEngage = rng.nextBool(); t.eqEngage = rng.nextBool(); t.compEngage = rng.nextBool();
        t.curve = rng.nextBool(); t.rmsMode = rng.nextBool(); t.scFilter = rng.nextBool();
        t.satEngage = rng.nextBool(); t.hexEngage = rng.nextBool();
        t.dryEqMute = rng.nextBool(); t.compMute = rng.nextBool(); t.satMute = rng.nextBool();
        t.ceiling = rng.nextBool();
        s.setTargets(t);

        for (int n = 0; n < kBlock; ++n) {
            const float v = rng.nextFloat()*2.0f - 1.0f;
            main.setSample(0, n, v); main.setSample(1, n, rng.nextFloat()*2.0f-1.0f);
        }
        sc.clear();
        s.processBlock(main, &sc);
        for (int ch = 0; ch < 2; ++ch)
            for (int n = 0; n < kBlock; ++n) {
                const float y = main.getSample(ch, n);
                REQUIRE(std::isfinite(y));
                REQUIRE(std::abs(y) < 50.0f);
            }
    }
}
