#include "PluginProcessor.h"

SmoothRideAudioProcessor::SmoothRideAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts(*this, nullptr, "SmoothRideState", createLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout SmoothRideAudioProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&](const char* id, const char* name, float lo, float hi, float def, float step) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
                        juce::NormalisableRange<float>(lo, hi, step), def));
    };
    add("target", "Target RMS (dB)", -30.0f, -8.0f, -15.0f, 0.1f);
    add("floor", "Floor (dB)", -90.0f, -30.0f, -60.0f, 0.1f);
    add("range", "Ride Range (+/- dB)", 0.0f, 12.0f, 6.0f, 0.1f);
    add("speed", "Ride Speed (ms)", 100.0f, 10000.0f, 1800.0f, 10.0f);
    add("mix", "Mix (%)", 0.0f, 100.0f, 65.0f, 1.0f);
    add("output", "Output Gain (dB)", -12.0f, 12.0f, 6.0f, 0.1f);
    add("ceiling", "Safety Ceiling (dBFS)", -12.0f, -0.1f, -1.0f, 0.1f);
    return layout;
}
void SmoothRideAudioProcessor::prepareToPlay(double sr, int) { rider.prepare(sr); setLatencySamples(0); }
bool SmoothRideAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto input = l.getMainInputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
           && l.getMainOutputChannelSet() == input;
}
void SmoothRideAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(getTotalNumInputChannels(), buffer.getNumChannels());
    if (channels < 1) { buffer.clear(); return; }
    for (int c = channels; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
    auto p = [&](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    smoothride::Settings settings;
    settings.targetDb = p("target"); settings.floorDb = p("floor"); settings.rangeDb = p("range");
    settings.speedMs = p("speed"); settings.mix = p("mix") / 100.0f; settings.outputDb = p("output");
    settings.ceilingDb = p("ceiling");
    rider.setSettings(settings);
    rider.process(buffer.getWritePointer(0), channels == 2 ? buffer.getWritePointer(1) : nullptr,
                  buffer.getNumSamples(), channels == 2);
}
juce::AudioProcessorEditor* SmoothRideAudioProcessor::createEditor() {
    return new juce::GenericAudioProcessorEditor(*this);
}
void SmoothRideAudioProcessor::getStateInformation(juce::MemoryBlock& b) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, b);
}
void SmoothRideAudioProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SmoothRideAudioProcessor(); }
