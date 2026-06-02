#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace holdover {

HoldoverProcessor::HoldoverProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input",      juce::AudioChannelSet::stereo(), true)
        .withOutput("Output",    juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain",  juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "Parameters", params::createLayout()) {}

void HoldoverProcessor::prepareToPlay(double, int) {}

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
    // Passthrough for now; clear any output channels beyond the input count.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* HoldoverProcessor::createEditor() {
    return new HoldoverEditor(*this);
}

void HoldoverProcessor::getStateInformation(juce::MemoryBlock& destData) {
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void HoldoverProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

} // namespace holdover

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new holdover::HoldoverProcessor();
}
