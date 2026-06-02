#include "ChannelStrip.h"

namespace holdover {

using juce::dsp::Oversampling;

void ChannelStrip::prepare(double sampleRate, int maxBlockSize, int numChannels) noexcept {
    sr_ = sampleRate;
    numCh_ = juce::jlimit(1, 2, numChannels);
    const double osSr = sampleRate * (1 << kFactor);

    for (auto& f : filter_) f.prepare(osSr);
    for (auto& e : eq_)     e.prepare(osSr);
    comp_.prepare(osSr, maxBlockSize * (1 << kFactor));
    for (auto& d : drive_)  d.prepare(osSr);
    for (auto& c : ceil_)   c.prepare(osSr);

    os_ = std::make_unique<Oversampling<float>>(
        (size_t) numCh_, kFactor, Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    scOs_ = std::make_unique<Oversampling<float>>(
        (size_t) numCh_, kFactor, Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    os_->initProcessing((size_t) maxBlockSize);
    scOs_->initProcessing((size_t) maxBlockSize);
    latencySamples_ = (int) std::round(os_->getLatencyInSamples());

    scBaseBuffer_.setSize(numCh_, maxBlockSize); // preallocated; reused each block
    reset();
}

void ChannelStrip::reset() noexcept {
    for (auto& f : filter_) f.reset();
    for (auto& e : eq_) e.reset();
    comp_.reset();
    for (auto& d : drive_) d.reset();
    for (auto& c : ceil_) c.reset();
    if (os_) os_->reset();
    if (scOs_) scOs_->reset();
    satActivity_ = 0.0f;
}

void ChannelStrip::setTargets(const Targets& t) noexcept { t_ = t; haveTargets_ = true; }

void ChannelStrip::applyTargets() noexcept {
    input_.setGains(t_.gainLpos, t_.gainRpos, t_.inputPos);
    for (auto& f : filter_) { f.setHpf(t_.hpfFreq, t_.hpfPeak); f.setLpf(t_.lpfFreq, t_.lpfPeak); }
    for (auto& e : eq_) {
        e.setBass(t_.bassDb, t_.bassFreq);
        e.setPresence(t_.presenceDb, t_.presenceFreq);
        e.setTreble(t_.trebleDb, t_.trebleFreq);
    }
    comp_.setThreshold(t_.thresholdDb);
    comp_.setBehavior(t_.behaviorPos);
    comp_.setMakeup(t_.makeupPos);
    comp_.setTiming(t_.attackIdx, t_.releaseIdx);
    comp_.setRmsMode(t_.rmsMode);
    comp_.setScFilter(t_.scFilter);
    for (auto& d : drive_) {
        d.setDrive(t_.drivePos); d.setMas(t_.masMode);
        d.setSat(t_.satEngage); d.setHex(t_.hexEngage); d.setCurve(t_.curve);
    }
    matrix_.setDryEq(t_.dryEqFeedPos, t_.dryEqMute);
    matrix_.setComp(t_.compFeedPos, t_.compMute);
    matrix_.setSat(t_.satFeedPos, t_.satMute);
    for (auto& c : ceil_) c.setEngaged(t_.ceiling);
    output_.setLevel(t_.outputPos);
}

void ChannelStrip::processBlock(juce::AudioBuffer<float>& main,
                                const juce::AudioBuffer<float>* sidechain) noexcept {
    if (!haveTargets_) return;
    applyTargets();

    const int numSamples = main.getNumSamples();

    // Input stage at base rate (== inputFaderOut).
    for (int n = 0; n < numSamples; ++n) {
        float l = main.getSample(0, n);
        float r = (numCh_ > 1) ? main.getSample(1, n) : l;
        input_.processStereo(l, r);
        main.setSample(0, n, l);
        if (numCh_ > 1) main.setSample(1, n, r);
    }

    // Fill preallocated base-rate sidechain scratch (zeros if not connected).
    scBaseBuffer_.clear();
    if (sidechain != nullptr)
        for (int ch = 0; ch < juce::jmin(numCh_, sidechain->getNumChannels()); ++ch)
            scBaseBuffer_.copyFrom(ch, 0, *sidechain, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> mainBlock(main);
    juce::dsp::AudioBlock<float> scBaseBlock =
        juce::dsp::AudioBlock<float>(scBaseBuffer_).getSubBlock(0, (size_t) numSamples);
    auto up   = os_->processSamplesUp(mainBlock);
    auto upSc = scOs_->processSamplesUp(scBaseBlock);

    processOversampled(up, upSc);

    os_->processSamplesDown(mainBlock);

    // Output stage at base rate.
    for (int n = 0; n < numSamples; ++n) {
        float l = main.getSample(0, n);
        float r = (numCh_ > 1) ? main.getSample(1, n) : l;
        output_.processStereo(l, r);
        main.setSample(0, n, l);
        if (numCh_ > 1) main.setSample(1, n, r);
    }

    meters_.setSaturation(juce::jlimit(0.0f, 1.0f, satActivity_));
    meters_.setGainReductionDb(comp_.getGainReductionDb());
}

void ChannelStrip::processOversampled(juce::dsp::AudioBlock<float>& up,
                                      juce::dsp::AudioBlock<float>& upSc) noexcept {
    const int n = (int) up.getNumSamples();
    const bool stereo = numCh_ > 1;
    float satAccum = 0.0f;

    for (int i = 0; i < n; ++i) {
        const float inL = up.getSample(0, i);
        const float inR = stereo ? up.getSample(1, i) : inL;

        const float filtL = t_.filtEngage ? filter_[0].processSample(inL) : inL;
        const float filtR = stereo ? (t_.filtEngage ? filter_[1].processSample(inR) : inR) : filtL;
        const float eqOutL = eq_[0].processSample(t_.eqEngage ? filtL : inL);
        const float eqOutR = stereo ? eq_[1].processSample(t_.eqEngage ? filtR : inR) : eqOutL;
        const float serialAfterEqL = t_.eqEngage ? eqOutL : filtL;
        const float serialAfterEqR = t_.eqEngage ? eqOutR : filtR;

        const float scExtL = upSc.getNumChannels() > 0 ? upSc.getSample(0, i) : 0.0f;
        const float scExtR = (stereo && upSc.getNumChannels() > 1) ? upSc.getSample(1, i) : scExtL;
        const float scL = (t_.scSource == 0) ? inL : (t_.scSource == 2) ? eqOutL : scExtL;
        const float scR = (t_.scSource == 0) ? inR : (t_.scSource == 2) ? eqOutR : scExtR;

        float compOutL = t_.compEngage ? serialAfterEqL : inL;
        float compOutR = t_.compEngage ? serialAfterEqR : inR;
        comp_.processStereo(compOutL, compOutR, scL, scR);

        const float serialAfterCompL = t_.compEngage ? compOutL : serialAfterEqL;
        const float serialAfterCompR = t_.compEngage ? compOutR : serialAfterEqR;

        const float driveOutL = drive_[0].processSample(serialAfterCompL);
        const float driveOutR = stereo ? drive_[1].processSample(serialAfterCompR) : driveOutL;

        const float dryEqL = (t_.dryEqSource == 0) ? inL : eqOutL;
        const float dryEqR = (t_.dryEqSource == 0) ? inR : eqOutR;
        float mixL = matrix_.mix(dryEqL, compOutL, driveOutL);
        float mixR = matrix_.mix(dryEqR, compOutR, driveOutR);

        const float outL = ceil_[0].processSample(mixL);
        const float outR = stereo ? ceil_[1].processSample(mixR) : outL;

        up.setSample(0, i, outL);
        if (stereo) up.setSample(1, i, outR);

        satAccum = juce::jmax(satAccum, std::abs(driveOutL - serialAfterCompL),
                                        std::abs(driveOutR - serialAfterCompR));
    }
    satActivity_ = satAccum;
}

} // namespace holdover
