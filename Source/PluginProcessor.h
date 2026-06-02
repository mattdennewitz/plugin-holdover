#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/ChannelStrip.h"
#include "presets/FactoryPresets.h"

namespace holdover {

class HoldoverProcessor : public juce::AudioProcessor {
public:
    HoldoverProcessor();
    ~HoldoverProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Holdover"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    int uiWidth = 900, uiHeight = 700;

    const MeterState& getMeters() const noexcept { return strip_.meters(); }

private:
    ChannelStrip strip_;
    int currentProgram_ = 0;

    juce::SmoothedValue<float> smGainL_, smGainR_, smInput_, smOutput_, smDrive_, smMakeup_,
        smThreshold_, smBehavior_, smDryEq_, smComp_, smSat_, smPresenceDb_, smBassDb_, smTrebleDb_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>
        smHpf_, smLpf_, smPresenceFreq_;

    std::atomic<float>* p(const char* id) { return apvts.getRawParameterValue(id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HoldoverProcessor)
};

} // namespace holdover
