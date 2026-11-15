#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <chrono>

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
    add("target", "Target RMS (dB)", -30.0f, -8.0f, -12.0f, 0.1f);
    add("floor", "Floor (dB)", -90.0f, -30.0f, -60.0f, 0.1f);
    add("range", "Max Boost (dB)", 0.0f, 12.0f, 6.0f, 0.1f);
    add("maxCut", "Max Cut (dB)", 0.0f, 12.0f, 3.0f, 0.1f);
    add("speed", "Gain Up Speed (ms)", 100.0f, 30000.0f, 3000.0f, 10.0f);
    add("downSpeed", "Gain Down Speed (ms)", 50.0f, 10000.0f, 650.0f, 10.0f);
    add("mix", "Mix (%)", 0.0f, 100.0f, 65.0f, 1.0f);
    add("output", "Output Gain (dB)", -12.0f, 12.0f, 6.0f, 0.1f);
    add("ceiling", "Safety Ceiling (dBFS)", -12.0f, -0.1f, -1.0f, 0.1f);
    add("gateHyst", "Floor Hysteresis (dB)", 0.0f, 12.0f, 0.0f, 0.5f);
    add("lowProtect", "Soft Floor (0=Off, 1=On)", 0.0f, 1.0f, 0.0f, 1.0f);
    return layout;
}
void SmoothRideAudioProcessor::prepareToPlay(double sr, int) {
    currentSampleRate = sr > 0.0 ? sr : 48000.0;
    rider.prepare(currentSampleRate);
    performance.reset();
    averageBlockLoad.store(0.0f, std::memory_order_relaxed);
    maximumBlockLoad.store(0.0f, std::memory_order_relaxed);
    callbackMicroseconds.store(0.0f, std::memory_order_relaxed);
    setLatencySamples(0);
}
bool SmoothRideAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto input = l.getMainInputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
           && l.getMainOutputChannelSet() == input;
}
void SmoothRideAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    const auto callbackStart = std::chrono::steady_clock::now();
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(getTotalNumInputChannels(), buffer.getNumChannels());
    if (channels < 1) { buffer.clear(); return; }
    for (int c = channels; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
    auto p = [&](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    smoothride::Settings settings;
    settings.targetDb = p("target"); settings.floorDb = p("floor"); settings.maxBoostDb = p("range"); settings.maxCutDb = p("maxCut");
    settings.upMs = p("speed"); settings.downMs = p("downSpeed"); settings.mix = p("mix") / 100.0f; settings.outputDb = p("output");
    settings.ceilingDb = p("ceiling");
    settings.gateHysteresisDb = p("gateHyst");
    settings.lowLevelProtect = p("lowProtect");
    float inPeak = 0.0f;
    double inputPower = 0.0;
    for (int c = 0; c < channels; ++c) {
        inPeak = juce::jmax(inPeak, buffer.getMagnitude(c, 0, buffer.getNumSamples()));
        const float* samples = buffer.getReadPointer(c);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            const double x = std::isfinite(samples[i]) ? samples[i] : 0.0;
            inputPower += x*x;
        }
    }
    const int count = channels * buffer.getNumSamples();
    const float blockRms = count > 0 ? (float) std::sqrt(inputPower / count) : 0.0f;
    rider.setSettings(settings);
    rider.process(buffer.getWritePointer(0), channels == 2 ? buffer.getWritePointer(1) : nullptr,
                  buffer.getNumSamples(), channels == 2);
    float outPeak = 0.0f;
    for (int c = 0; c < channels; ++c)
        outPeak = juce::jmax(outPeak, buffer.getMagnitude(c, 0, buffer.getNumSamples()));
    inputPeak.store(inPeak, std::memory_order_relaxed);
    inputBlockRms.store(blockRms, std::memory_order_relaxed);
    outputPeak.store(outPeak, std::memory_order_relaxed);
    rideDb.store(rider.currentRideDb(), std::memory_order_relaxed);
    limiterReductionDb.store(rider.currentPeakReductionDb(), std::memory_order_relaxed);
    detectedRmsDb.store(rider.currentDetectedRmsDb(), std::memory_order_relaxed);
    requestedRideDb.store(rider.currentRequestedRideDb(), std::memory_order_relaxed);
    riderActive.store(rider.isRidingActive(), std::memory_order_relaxed);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - callbackStart).count();
    performance.record(seconds, buffer.getNumSamples(), currentSampleRate);
    averageBlockLoad.store(performance.averagePercent(), std::memory_order_relaxed);
    maximumBlockLoad.store(performance.maximumPercent(), std::memory_order_relaxed);
    callbackMicroseconds.store(performance.callbackMicroseconds(), std::memory_order_relaxed);
}
juce::AudioProcessorEditor* SmoothRideAudioProcessor::createEditor() {
    return new SmoothRideAudioProcessorEditor(*this);
}
namespace {
const std::array<const char*, 11> snapshotIDs {{
    "target", "floor", "range", "maxCut", "speed", "downSpeed", "mix",
    "output", "ceiling", "gateHyst", "lowProtect"
}};
const juce::Identifier userBankID("UserPresets"), presetID("UserPreset"), abID("ABSlot");

juce::ValueTree ensureUserBank(juce::ValueTree root) {
    auto bank = root.getChildWithName(userBankID);
    if (!bank.isValid()) {
        bank = juce::ValueTree(userBankID);
        root.addChild(bank, -1, nullptr);
    }
    return bank;
}

juce::ValueTree findNamed(juce::ValueTree bank, juce::Identifier type,
                          const juce::String& name) {
    for (int i = 0; i < bank.getNumChildren(); ++i) {
        auto node = bank.getChild(i);
        if (node.hasType(type) && node.getProperty("name").toString() == name)
            return node;
    }
    return {};
}

void saveSnapshot(juce::ValueTree node, const SmoothRideAudioProcessor::Snapshot& values) {
    for (size_t i = 0; i < snapshotIDs.size(); ++i)
        node.setProperty(juce::Identifier(snapshotIDs[i]), values[i], nullptr);
}

SmoothRideAudioProcessor::Snapshot readSnapshot(juce::ValueTree node,
                                                const SmoothRideAudioProcessor::Snapshot& fallback) {
    auto result = fallback;
    for (size_t i = 0; i < snapshotIDs.size(); ++i) {
        const juce::Identifier key(snapshotIDs[i]);
        if (node.hasProperty(key)) result[i] = (float) node.getProperty(key);
    }
    return result;
}
}

SmoothRideAudioProcessor::Snapshot SmoothRideAudioProcessor::captureParameters() const {
    Snapshot values {};
    for (size_t i = 0; i < snapshotIDs.size(); ++i) {
        if (auto* raw = apvts.getRawParameterValue(snapshotIDs[i]))
            values[i] = raw->load(std::memory_order_relaxed);
    }
    return values;
}

void SmoothRideAudioProcessor::applyParameters(const Snapshot& values) {
    // Run from the GUI message thread; parameter IDs remain compatible with v0.4.0.
    for (size_t i = 0; i < snapshotIDs.size(); ++i) {
        if (auto* param = apvts.getParameter(snapshotIDs[i])) {
            const auto normalized = param->convertTo0to1(values[i]);
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalized);
            param->endChangeGesture();
        }
    }
}

juce::StringArray SmoothRideAudioProcessor::listUserPresets() const {
    juce::StringArray names;
    auto bank = apvts.state.getChildWithName(userBankID);
    if (bank.isValid()) {
        for (int i = 0; i < bank.getNumChildren(); ++i) {
            auto child = bank.getChild(i);
            if (child.hasType(presetID)) names.add(child.getProperty("name").toString());
        }
    }
    names.sort(true);
    return names;
}

bool SmoothRideAudioProcessor::saveUserPreset(const juce::String& inputName) {
    const auto name = inputName.trim();
    if (name.isEmpty() || name.length() > 40) return false;
    auto bank = ensureUserBank(apvts.state);
    auto preset = findNamed(bank, presetID, name);
    if (!preset.isValid()) {
        preset = juce::ValueTree(presetID);
        preset.setProperty("name", name, nullptr);
        bank.addChild(preset, -1, nullptr);
    }
    saveSnapshot(preset, captureParameters());
    return true;
}

bool SmoothRideAudioProcessor::loadUserPreset(const juce::String& name) {
    auto preset = findNamed(apvts.state.getChildWithName(userBankID), presetID, name);
    if (!preset.isValid()) return false;
    applyParameters(readSnapshot(preset, captureParameters()));
    return true;
}

bool SmoothRideAudioProcessor::deleteUserPreset(const juce::String& name) {
    auto bank = apvts.state.getChildWithName(userBankID);
    auto preset = findNamed(bank, presetID, name);
    if (!preset.isValid()) return false;
    bank.removeChild(preset, nullptr);
    return true;
}

void SmoothRideAudioProcessor::captureAB(bool slotB) {
    auto bank = ensureUserBank(apvts.state);
    const juce::String name = slotB ? "B" : "A";
    auto node = findNamed(bank, abID, name);
    if (!node.isValid()) {
        node = juce::ValueTree(abID);
        node.setProperty("name", name, nullptr);
        bank.addChild(node, -1, nullptr);
    }
    saveSnapshot(node, captureParameters());
}

bool SmoothRideAudioProcessor::hasAB(bool slotB) const {
    return findNamed(apvts.state.getChildWithName(userBankID), abID,
                     slotB ? "B" : "A").isValid();
}

bool SmoothRideAudioProcessor::recallAB(bool slotB) {
    auto node = findNamed(apvts.state.getChildWithName(userBankID), abID,
                          slotB ? "B" : "A");
    if (!node.isValid()) return false;
    applyParameters(readSnapshot(node, captureParameters()));
    return true;
}

void SmoothRideAudioProcessor::getStateInformation(juce::MemoryBlock& b) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, b);
}
void SmoothRideAudioProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SmoothRideAudioProcessor(); }
