#pragma once
#include <JuceHeader.h>
#include "GainRider.h"
#include "PerformanceTelemetry.h"
#include <atomic>

class SmoothRideAudioProcessor final : public juce::AudioProcessor {
public:
    SmoothRideAudioProcessor();
    const juce::String getName() const override { return "SmoothRide"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState apvts;
    // User snapshots are stored within the host state, not on the audio thread.
    static constexpr int presetParameterCount = 13;
    using Snapshot = std::array<float, presetParameterCount>;
    Snapshot captureParameters() const;
    void applyParameters(const Snapshot& values);
    juce::StringArray listUserPresets() const;
    bool saveUserPreset(const juce::String& name);
    bool loadUserPreset(const juce::String& name);
    bool deleteUserPreset(const juce::String& name);
    // Portable .smoothridepreset XML; functions run only on the message thread.
    bool exportUserPreset(const juce::String& name, const juce::File& file, juce::String& error) const;
    bool importUserPreset(const juce::File& file, juce::String& error);
    void captureAB(bool slotB);
    bool recallAB(bool slotB);
    bool hasAB(bool slotB) const;
    bool calibrateAB(bool slotB);
    float getABCalibration(bool slotB) const;
    void setABMatching(bool enabled);
    bool isABMatching() const noexcept { return abMatchEnabled.load(std::memory_order_relaxed); }
    // Output-level estimate for manual A/B reference calibration, not integrated LUFS.
    float getRecentOutputRmsDb() const noexcept { return recentOutputRmsDb.load(std::memory_order_relaxed); }
    float getABMatchingDb() const noexcept { return comparisonGainTargetDb.load(std::memory_order_relaxed); }


    // Single audio-thread writer, GUI-thread readers. Telemetry does not affect DSP.
    float getInputBlockRms() const noexcept { return inputBlockRms.load(std::memory_order_relaxed); }
    float getInputPeak() const noexcept { return inputPeak.load(std::memory_order_relaxed); }
    float getOutputPeak() const noexcept { return outputPeak.load(std::memory_order_relaxed); }
    float getRideDb() const noexcept { return rideDb.load(std::memory_order_relaxed); }
    float getLimiterReductionDb() const noexcept { return limiterReductionDb.load(std::memory_order_relaxed); }
    float getDetectedRmsDb() const noexcept { return detectedRmsDb.load(std::memory_order_relaxed); }
    float getRequestedRideDb() const noexcept { return requestedRideDb.load(std::memory_order_relaxed); }
    float getEffectiveFloorDb() const noexcept { return effectiveFloorDb.load(std::memory_order_relaxed); }
    bool getRiderActive() const noexcept { return riderActive.load(std::memory_order_relaxed); }
    bool getSilenceBlocked() const noexcept { return silenceBlocked.load(std::memory_order_relaxed); }
    bool getNoiseBlocked() const noexcept { return noiseBlocked.load(std::memory_order_relaxed); }
    float getNoiseReferenceDb() const noexcept { return noiseReferenceDb.load(std::memory_order_relaxed); }
    smoothride::DSPSnapshot getDSPSnapshot() const noexcept { return publishedDSP.read(); }
    void rememberDSPSnapshot(const smoothride::DSPSnapshot& s) const noexcept { publishedDSP.remember(s); }
    float getAverageBlockLoadPercent() const noexcept { return averageBlockLoad.load(std::memory_order_relaxed); }
    float getMaximumBlockLoadPercent() const noexcept { return maximumBlockLoad.load(std::memory_order_relaxed); }
    float getLastCallbackMicroseconds() const noexcept { return callbackMicroseconds.load(std::memory_order_relaxed); }
    float getRollingPeakLoadPercent() const noexcept { return rollingPeakLoad.load(std::memory_order_relaxed); }
    std::uint64_t getDSPSpikeCount() const noexcept { return dspSpikeCount.load(std::memory_order_relaxed); }
    void requestDSPReset() noexcept { dspResetRevision.fetch_add(1u, std::memory_order_release); }
    void setDSPSpikeThreshold(float percent) noexcept { dspSpikeThreshold.store(percent, std::memory_order_relaxed); }
    float getDSPSpikeThreshold() const noexcept { return dspSpikeThreshold.load(std::memory_order_relaxed); }
    // Receives no allocations, locks or GUI callbacks on the audio thread.

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    smoothride::GainRider rider, previousRider;
    smoothride::Settings lastAudioSettings;
    juce::AudioBuffer<float> transitionBuffer;
    int transitionSamplesLeft = 0, transitionDuration = 1;
    int preparedBufferSize = 0;
    std::atomic<unsigned int> snapshotVersion {0};
    unsigned int lastSnapshotVersion = 0;
    std::atomic<bool> abMatchEnabled {false};
    std::atomic<float> comparisonGainTargetDb {0.f}, recentOutputRmsDb {-100.f};
    float previousCompareGainLinear = 1.f, currentCompareGainLinear = 1.f;
    double recentPower = 1.e-12;

    smoothride::PerformanceTelemetry performance;
    smoothride::PublishedDSPTelemetry publishedDSP;
    double currentSampleRate = 48000.0;
    std::atomic<float> averageBlockLoad {0.0f}, maximumBlockLoad {0.0f}, callbackMicroseconds {0.0f};
    std::atomic<float> rollingPeakLoad {0.0f}, dspSpikeThreshold {10.0f};
    std::atomic<std::uint64_t> dspSpikeCount {0};
    std::atomic<unsigned int> dspResetRevision {0};
    unsigned int lastDSPResetRevision = 0;

    std::atomic<float> inputPeak {0.0f}, outputPeak {0.0f}, inputBlockRms {0.0f};
    std::atomic<float> rideDb {0.0f}, limiterReductionDb {0.0f};
    std::atomic<float> detectedRmsDb {-160.0f}, requestedRideDb {0.0f}, effectiveFloorDb {-60.0f};
    std::atomic<bool> riderActive {false}, silenceBlocked {false}, noiseBlocked {false};
    std::atomic<float> noiseReferenceDb {-120.0f};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmoothRideAudioProcessor)
};
