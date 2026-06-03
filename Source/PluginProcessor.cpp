#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace holdover {

HoldoverProcessor::HoldoverProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input",      juce::AudioChannelSet::stereo(), true)
        .withOutput("Output",    juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain",  juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "Parameters", params::createLayout()) {
    currentPresetName = presetManager.getCurrentName();   // "Init / Flat" on first launch
}

void HoldoverProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    const int numCh = juce::jmax(1, getMainBusNumInputChannels());
    strip_.prepare(sampleRate, samplesPerBlock, numCh);
    setLatencySamples(strip_.getLatencySamples());

    constexpr double ramp = 0.008; // 8 ms
    auto initLin = [&](juce::SmoothedValue<float>& sv, const char* id) {
        sv.reset(sampleRate, ramp);
        sv.setCurrentAndTargetValue(p(id)->load());
    };
    initLin(smGainL_, "gainL");   initLin(smGainR_, "gainR");   initLin(smInput_, "input");
    initLin(smOutput_, "output"); initLin(smDrive_, "drive");   initLin(smMakeup_, "makeup");
    initLin(smCharacter_, "character");
    initLin(smThreshold_, "threshold"); initLin(smBehavior_, "behavior");
    initLin(smDryEq_, "dryEqFeedLevel"); initLin(smComp_, "compFeedLevel"); initLin(smSat_, "satFeedLevel");
    initLin(smPresenceDb_, "presenceDb"); initLin(smBassDb_, "bassDb"); initLin(smTrebleDb_, "trebleDb");

    auto initLog = [&](juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>& sv, const char* id) {
        sv.reset(sampleRate, ramp);
        sv.setCurrentAndTargetValue(juce::jmax(1.0f, p(id)->load()));
    };
    initLog(smHpf_, "hpfFreq"); initLog(smLpf_, "lpfFreq"); initLog(smPresenceFreq_, "presenceFreq");
}

bool HoldoverProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != main)
        return false;
    // Sidechain may be disabled, mono, or stereo.
    const auto sc = layouts.getChannelSet(true, 1);
    if (!sc.isDisabled() && sc != juce::AudioChannelSet::mono()
                         && sc != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void HoldoverProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int blk = buffer.getNumSamples();

    auto adv = [blk](juce::SmoothedValue<float>& sv, std::atomic<float>* param) {
        sv.setTargetValue(param->load());
        return sv.skip(blk);
    };
    auto advLog = [blk](juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>& sv,
                        std::atomic<float>* param) {
        sv.setTargetValue(juce::jmax(1.0f, param->load()));
        return sv.skip(blk);
    };

    static constexpr float bassF[3]   = { 50.0f, 150.0f, 300.0f };
    static constexpr float trebleF[3] = { 5000.0f, 10000.0f, 15000.0f };

    ChannelStrip::Targets t {};
    t.gainLpos = adv(smGainL_, p("gainL"));
    t.gainRpos = adv(smGainR_, p("gainR"));
    t.inputPos = adv(smInput_, p("input"));
    t.outputPos = adv(smOutput_, p("output"));
    t.drivePos = adv(smDrive_, p("drive"));
    t.makeupPos = adv(smMakeup_, p("makeup"));
    t.characterPos = adv(smCharacter_, p("character"));
    t.thresholdDb = adv(smThreshold_, p("threshold"));
    t.behaviorPos = adv(smBehavior_, p("behavior"));
    t.dryEqFeedPos = adv(smDryEq_, p("dryEqFeedLevel"));
    t.compFeedPos = adv(smComp_, p("compFeedLevel"));
    t.satFeedPos = adv(smSat_, p("satFeedLevel"));
    t.presenceDb = adv(smPresenceDb_, p("presenceDb"));
    t.bassDb = adv(smBassDb_, p("bassDb"));
    t.trebleDb = adv(smTrebleDb_, p("trebleDb"));
    t.hpfFreq = advLog(smHpf_, p("hpfFreq"));
    t.lpfFreq = advLog(smLpf_, p("lpfFreq"));
    t.presenceFreq = advLog(smPresenceFreq_, p("presenceFreq"));
    t.hpfPeak = p("hpfPeak")->load();
    t.lpfPeak = p("lpfPeak")->load();
    t.bassFreq = bassF[(int) p("bassFreq")->load()];
    t.trebleFreq = trebleF[(int) p("trebleFreq")->load()];
    t.attackIdx = (int) p("attack")->load();
    t.releaseIdx = (int) p("release")->load();
    t.masMode = (int) p("masMode")->load();
    t.scSource = (int) p("scSource")->load();
    t.dryEqSource = (int) p("dryEqSource")->load();
    t.filtEngage = p("filtEngage")->load() > 0.5f;
    t.eqEngage   = p("eqEngage")->load()   > 0.5f;
    t.compEngage = p("compEngage")->load() > 0.5f;
    t.curve      = p("curve")->load()      > 0.5f;
    t.rmsMode    = p("rmsMode")->load()    > 0.5f;
    t.scFilter   = p("scFilter")->load()   > 0.5f;
    t.satEngage  = p("satEngage")->load()  > 0.5f;
    t.hexEngage  = p("hexEngage")->load()  > 0.5f;
    t.dryEqMute  = p("dryEqMute")->load()  > 0.5f;
    t.compMute   = p("compMute")->load()   > 0.5f;
    t.satMute    = p("satMute")->load()    > 0.5f;
    t.ceiling    = p("ceiling")->load()    > 0.5f;
    strip_.setTargets(t);

    // REALTIME-SAFE sidechain handling: bus buffers are non-owning views.
    // Use direct-initialization (auto) so getBusBuffer's returned view is NOT
    // copy-assigned into an owning AudioBuffer (which would allocate). Do NOT
    // write `AudioBuffer<float> sc; sc = getBusBuffer(...);` — that allocates.
    auto mainBus = getBusBuffer(buffer, true, 0);
    const bool scEnabled = getBus(true, 1) != nullptr && getBus(true, 1)->isEnabled();
    if (scEnabled) {
        auto scBus = getBusBuffer(buffer, true, 1);
        strip_.processBlock(mainBus, &scBus);
    } else {
        strip_.processBlock(mainBus, nullptr);
    }
}

juce::AudioProcessorEditor* HoldoverProcessor::createEditor() {
    return new HoldoverEditor(*this);
}

int HoldoverProcessor::getNumPrograms() { return presetManager.getNumFactory(); }

int HoldoverProcessor::getCurrentProgram() {
    // The host menu is factory-only; a loaded user preset reports as the clamped
    // factory index.
    return juce::jlimit(0, getNumPrograms() - 1, presetManager.getCurrentIndex());
}

void HoldoverProcessor::setCurrentProgram(int index) {
    presetManager.loadByIndex(index);
    currentPresetName = presetManager.getCurrentName();
}

const juce::String HoldoverProcessor::getProgramName(int index) {
    if (index >= 0 && index < getNumPrograms())
        return presets::all()[(size_t) index].name;
    return {};
}

void HoldoverProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::ValueTree root("HoldoverState");
    root.appendChild(apvts.copyState(), nullptr);
    juce::ValueTree ui("UI");
    ui.setProperty("width", uiWidth, nullptr);
    ui.setProperty("height", uiHeight, nullptr);
    ui.setProperty("preset", currentPresetName, nullptr);
    root.appendChild(ui, nullptr);
    if (auto xml = root.createXml())
        copyXmlToBinary(*xml, destData);
}

void HoldoverProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml) return;
    auto root = juce::ValueTree::fromXml(*xml);
    if (!root.isValid()) return;

    // Pre-1.0 format: state is wrapped in "HoldoverState". Unrecognized or older
    // blobs simply find no matching children and are ignored (no migration needed).
    auto paramState = root.getChildWithName(apvts.state.getType());
    if (paramState.isValid()) {
        apvts.replaceState(paramState);
        // Force-apply each stored value to its AudioProcessorParameter directly.
        // This is necessary because APVTS::replaceState skips parameters whose
        // denormalised value hasn't changed (e.g. a snapped bool whose unnormalisedValue
        // is already 0.0 but whose raw AudioParameterBool::value is still a fuzzed float).
        for (int i = 0; i < paramState.getNumChildren(); ++i) {
            const auto child = paramState.getChild(i);
            if (auto* param = apvts.getParameter(child.getProperty("id").toString())) {
                const float stored = child.getProperty("value", param->getDefaultValue());
                const float normalised = param->convertTo0to1(stored);
                param->setValueNotifyingHost(normalised);
            }
        }
    }

    auto ui = root.getChildWithName("UI");
    if (ui.isValid()) {
        uiWidth  = (int) ui.getProperty("width", uiWidth);
        uiHeight = (int) ui.getProperty("height", uiHeight);
        currentPresetName = ui.getProperty("preset", currentPresetName).toString();
    }
}

} // namespace holdover

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new holdover::HoldoverProcessor();
}
