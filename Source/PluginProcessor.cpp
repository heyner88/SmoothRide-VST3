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
    add("adaptiveFloor", "Adaptive Floor (0=Off, 1=On)", 0.0f, 1.0f, 0.0f, 1.0f);
    add("noiseGuard", "Experimental Noise Guard (0=Off, 1=On)", 0.0f, 1.0f, 0.0f, 1.0f);
    return layout;
}
void SmoothRideAudioProcessor::prepareToPlay(double sr, int) {
    currentSampleRate = sr > 0.0 ? sr : 48000.0;
    rider.prepare(currentSampleRate);
    previousRider.prepare(currentSampleRate);
    transitionDuration = juce::jmax(1, (int) std::round(currentSampleRate * 0.08));
    transitionSamplesLeft = 0;
    // Scratch allocated outside the real-time callback.
    preparedBufferSize = 16384;
    transitionBuffer.setSize(2, preparedBufferSize, false, false, true);
    transitionBuffer.clear();
    recentPower = 1.e-12;
    recentOutputRmsDb.store(-100.f, std::memory_order_relaxed);
    currentCompareGainLinear = previousCompareGainLinear = 1.f;
    lastSnapshotVersion = snapshotVersion.load(std::memory_order_relaxed);
    performance.reset();
    publishedDSP.publish(performance);
    lastDSPResetRevision = dspResetRevision.load(std::memory_order_acquire);
    rollingPeakLoad.store(0.f, std::memory_order_relaxed);
    dspSpikeCount.store(0, std::memory_order_relaxed);
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
    // GUI requests a reset via an atomic revision. The audio thread alone owns telemetry.
    const auto resetRevision = dspResetRevision.load(std::memory_order_acquire);
    if (resetRevision != lastDSPResetRevision) {
        lastDSPResetRevision = resetRevision;
        performance.reset();
        publishedDSP.publish(performance);
        averageBlockLoad.store(0.f, std::memory_order_relaxed);
        maximumBlockLoad.store(0.f, std::memory_order_relaxed);
        rollingPeakLoad.store(0.f, std::memory_order_relaxed);
        dspSpikeCount.store(0, std::memory_order_relaxed);
        callbackMicroseconds.store(0.f, std::memory_order_relaxed);
    }
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
    settings.adaptiveFloor = p("adaptiveFloor");
    settings.noiseGuard = p("noiseGuard");
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
    // A/B recall signals an 80 ms dual-engine transition, preserving the previous
    // riding state and its exact settings. No allocations occur inside processBlock.
    const auto revision = snapshotVersion.load(std::memory_order_acquire);
    if (revision != lastSnapshotVersion) {
        lastSnapshotVersion = revision;
        previousRider = rider;
        previousRider.setSettings(lastAudioSettings);
        previousCompareGainLinear = currentCompareGainLinear;
        transitionSamplesLeft = transitionDuration;
    }
    const float requestedCompensationDb = abMatchEnabled.load(std::memory_order_relaxed)
                                   ? comparisonGainTargetDb.load(std::memory_order_relaxed) : 0.f;
    lastAudioSettings = settings;
    const bool canCrossfade = transitionSamplesLeft > 0 && buffer.getNumSamples() <= preparedBufferSize;
    if (canCrossfade) {
        for (int c=0; c<channels; ++c)
            transitionBuffer.copyFrom(c, 0, buffer, c, 0, buffer.getNumSamples());
        previousRider.process(transitionBuffer.getWritePointer(0),
             channels == 2 ? transitionBuffer.getWritePointer(1) : nullptr,
             buffer.getNumSamples(), channels == 2);
    }
    rider.setSettings(settings);
    rider.process(buffer.getWritePointer(0), channels == 2 ? buffer.getWritePointer(1) : nullptr,
                  buffer.getNumSamples(), channels == 2);
    // Apply optional comparison-only gain, independently of the saved DSP parameters.
    // Crossfade from both full processing engines rather than abruptly switching gain.
    // Gain matching is bounded to +/-6 dB per slot and fades into place over 80 ms.
    double finalPower = 0.0;
    const float targetComp = juce::jlimit(-6.f, 0.f, requestedCompensationDb);
    const float targetLinear = std::pow(10.f, targetComp / 20.f);
    const float compensationAlpha = 1.f - std::exp(-1.f/(float)(currentSampleRate*0.08));
    const float oldComp = previousCompareGainLinear;
    for (int i=0; i<buffer.getNumSamples(); ++i) {
        // Smooth gain in linear amplitude, with no transcendental math per sample.
        currentCompareGainLinear += (targetLinear-currentCompareGainLinear)*compensationAlpha;
        const float newComp = currentCompareGainLinear;
        const float fraction = canCrossfade ? juce::jlimit(0.f, 1.f,
              1.f - (float)transitionSamplesLeft / (float)transitionDuration) : 1.f;
        for (int c=0; c<channels; ++c) {
            const float newValue = buffer.getSample(c,i) * newComp;
            const float oldValue = canCrossfade ? transitionBuffer.getSample(c,i) * oldComp : 0.f;
            const float output = canCrossfade ? oldValue*(1.f-fraction) + newValue*fraction : newValue;
            buffer.setSample(c,i, output);
            finalPower += (double)output*(double)output;
        }
        if (transitionSamplesLeft > 0) --transitionSamplesLeft;
    }
    if (transitionSamplesLeft == 0) previousCompareGainLinear = currentCompareGainLinear;
    const double meanPower = count>0 ? finalPower / (double)count : 0.0;
    const double smoothing = std::exp(-(double)buffer.getNumSamples()
                       / (currentSampleRate*2.0));
    recentPower = smoothing * recentPower + (1.0-smoothing)*meanPower;
    recentOutputRmsDb.store((float)(10.0*std::log10(std::max(1.e-16,recentPower))),
                             std::memory_order_relaxed);
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
    effectiveFloorDb.store(rider.currentEffectiveFloorDb(), std::memory_order_relaxed);
    riderActive.store(rider.isRidingActive(), std::memory_order_relaxed);
    silenceBlocked.store(rider.isSilenceBlocked(), std::memory_order_relaxed);
    noiseBlocked.store(rider.isNoiseBlocked(), std::memory_order_relaxed);
    noiseReferenceDb.store(rider.currentNoiseReferenceDb(), std::memory_order_relaxed);
    const auto callbackEnd = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(callbackEnd - callbackStart).count();
    const auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(callbackEnd.time_since_epoch()).count();
    performance.record(seconds, buffer.getNumSamples(), currentSampleRate, timestamp,
                       dspSpikeThreshold.load(std::memory_order_relaxed));
    averageBlockLoad.store(performance.averagePercent(), std::memory_order_relaxed);
    maximumBlockLoad.store(performance.maximumPercent(), std::memory_order_relaxed);
    rollingPeakLoad.store(performance.rollingPeakPercent(), std::memory_order_relaxed);
    dspSpikeCount.store(performance.spikes(), std::memory_order_relaxed);
    callbackMicroseconds.store(performance.callbackMicroseconds(), std::memory_order_relaxed);
    publishedDSP.publish(performance);
}
juce::AudioProcessorEditor* SmoothRideAudioProcessor::createEditor() {
    return new SmoothRideAudioProcessorEditor(*this);
}
namespace {
const std::array<const char*, 13> snapshotIDs {{
    "target", "floor", "range", "maxCut", "speed", "downSpeed", "mix",
    "output", "ceiling", "gateHyst", "lowProtect", "adaptiveFloor", "noiseGuard"
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
        else if (juce::String(snapshotIDs[i]) == "noiseGuard"
                 || juce::String(snapshotIDs[i]) == "adaptiveFloor") result[i] = 0.f;
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

bool SmoothRideAudioProcessor::exportUserPreset(const juce::String& name,
                                                  const juce::File& file,
                                                  juce::String& error) const {
    auto node = findNamed(apvts.state.getChildWithName(userBankID), presetID, name);
    if (!node.isValid()) { error = "Select a saved preset first"; return false; }
    juce::XmlElement xml("SmoothRidePreset");
    xml.setAttribute("version", 3);
    xml.setAttribute("name", name);
    for (const auto* id : snapshotIDs) {
        const juce::Identifier key(id);
        if (!node.hasProperty(key) && juce::String(id) != "noiseGuard"
            && juce::String(id) != "adaptiveFloor") {
            error = "Preset is missing a parameter"; return false;
        }
        xml.setAttribute(juce::String(id), node.hasProperty(key)
            ? (double)(float)node.getProperty(key) : 0.0);
    }
    if (!file.replaceWithText(xml.toString(), false, false, "\n")) {
        error = "Could not write preset file"; return false;
    }
    return true;
}

bool SmoothRideAudioProcessor::importUserPreset(const juce::File& file,
                                                  juce::String& error) {
    if (!file.existsAsFile() || file.getSize() > 65536) {
        error = "Missing or oversized preset file"; return false;
    }
    juce::XmlDocument doc(file);
    auto xml = doc.getDocumentElement();
    const int fileVersion = xml ? xml->getIntAttribute("version", -1) : -1;
    if (!xml || !xml->hasTagName("SmoothRidePreset") || (fileVersion != 1 && fileVersion != 2 && fileVersion != 3)) {
        error = "Unsupported preset format"; return false;
    }
    const auto name = xml->getStringAttribute("name").trim();
    if (name.isEmpty() || name.length() > 40) {
        error = "Invalid preset name"; return false;
    }
    auto bank = ensureUserBank(apvts.state);
    if (findNamed(bank, presetID, name).isValid()) {
        error = "Name already exists; rename or delete it first"; return false;
    }
    Snapshot values {};
    for (size_t i = 0; i < snapshotIDs.size(); ++i) {
        const juce::String id(snapshotIDs[i]);
        if (!xml->hasAttribute(id)) {
            // v0.5.x preset files have 11 parameters, no adaptive mode.
            if ((fileVersion == 1 && id == "adaptiveFloor")
                || (fileVersion <= 2 && id == "noiseGuard")) {
                values[i] = 0.f;
                continue;
            }
            error = "Missing parameter: " + id; return false;
        }
        const auto raw = xml->getStringAttribute(id);
        if (raw.isEmpty() || !raw.containsOnly("-+.0123456789eE")) {
            error = "Invalid numeric parameter: " + id; return false;
        }
        const double value = raw.getDoubleValue();
        const auto* parameter = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(id));
        if (!parameter || !std::isfinite(value)) {
            error = "Invalid parameter value: " + id; return false;
        }
        const auto range = parameter->range;
        if (value < range.start || value > range.end) {
            error = "Out-of-range parameter: " + id; return false;
        }
        values[i] = (float)value;
    }
    auto node = juce::ValueTree(presetID);
    node.setProperty("name", name, nullptr);
    saveSnapshot(node, values);
    bank.addChild(node, -1, nullptr);
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
    node.removeProperty("calDb", nullptr);
}

bool SmoothRideAudioProcessor::hasAB(bool slotB) const {
    return findNamed(apvts.state.getChildWithName(userBankID), abID,
                     slotB ? "B" : "A").isValid();
}

bool SmoothRideAudioProcessor::recallAB(bool slotB) {
    auto node = findNamed(apvts.state.getChildWithName(userBankID), abID,
                          slotB ? "B" : "A");
    if (!node.isValid()) return false;
    const auto bank = apvts.state.getChildWithName(userBankID);
    auto a = findNamed(bank, abID, "A");
    auto b = findNamed(bank, abID, "B");
    float matchDb = 0.f;
    if (abMatchEnabled.load(std::memory_order_relaxed)
        && a.isValid() && b.isValid() && a.hasProperty("calDb") && b.hasProperty("calDb")) {
        const float reference = juce::jmin((float)a.getProperty("calDb"), (float)b.getProperty("calDb"));
        const float slotLevel = (float)node.getProperty("calDb");
        // Attenuation-only prevents comparison matching from defeating the peak ceiling.
        matchDb = juce::jlimit(-6.f, 0.f, reference-slotLevel);
    }
    comparisonGainTargetDb.store(matchDb, std::memory_order_relaxed);
    applyParameters(readSnapshot(node, captureParameters()));
    snapshotVersion.fetch_add(1, std::memory_order_release);
    return true;
}

bool SmoothRideAudioProcessor::calibrateAB(bool slotB) {
    auto bank = apvts.state.getChildWithName(userBankID);
    auto node = findNamed(bank, abID, slotB ? "B" : "A");
    if (!node.isValid()) return false;
    node.setProperty("calDb", getRecentOutputRmsDb(), nullptr);
    return true;
}

float SmoothRideAudioProcessor::getABCalibration(bool slotB) const {
    auto node = findNamed(apvts.state.getChildWithName(userBankID), abID, slotB ? "B" : "A");
    return node.isValid() && node.hasProperty("calDb") ? (float)node.getProperty("calDb") : -160.f;
}

void SmoothRideAudioProcessor::setABMatching(bool enabled) {
    abMatchEnabled.store(enabled, std::memory_order_relaxed);
    apvts.state.setProperty("abMatchEnabled", enabled, nullptr);
    if (!enabled) comparisonGainTargetDb.store(0.f, std::memory_order_relaxed);
}

void SmoothRideAudioProcessor::getStateInformation(juce::MemoryBlock& b) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, b);
}
void SmoothRideAudioProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            abMatchEnabled.store((bool)apvts.state.getProperty("abMatchEnabled", false),
                                 std::memory_order_relaxed);
        }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SmoothRideAudioProcessor(); }
