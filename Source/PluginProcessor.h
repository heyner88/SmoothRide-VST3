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
    // Single audio-thread writer, GUI-thread readers. Telemetry does not affect DSP.
    float getInputBlockRms() const noexcept { return inputBlockRms.load(std::memory_order_relaxed); }
    float getInputPeak() const noexcept { return inputPeak.load(std::memory_order_relaxed); }
    float getOutputPeak() const noexcept { return outputPeak.load(std::memory_order_relaxed); }
    float getRideDb() const noexcept { return rideDb.load(std::memory_order_relaxed); }
    float getLimiterReductionDb() const noexcept { return limiterReductionDb.load(std::memory_order_relaxed); }
    float getDetectedRmsDb() const noexcept { return detectedRmsDb.load(std::memory_order_relaxed); }
    float getRequestedRideDb() const noexcept { return requestedRideDb.load(std::memory_order_relaxed); }
    bool getRiderActive() const noexcept { return riderActive.load(std::memory_order_relaxed); }
    float getAverageBlockLoadPercent() const noexcept { return averageBlockLoad.load(std::memory_order_relaxed); }
    float getMaximumBlockLoadPercent() const noexcept { return maximumBlockLoad.load(std::memory_order_relaxed); }
    float getLastCallbackMicroseconds() const noexcept { return callbackMicroseconds.load(std::memory_order_relaxed); }
    // Receives no allocations, locks or GUI callbacks on the audio thread.

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    smoothride::GainRider rider;
    smoothride::PerformanceTelemetry performance;
    double currentSampleRate = 48000.0;
    std::atomic<float> averageBlockLoad {0.0f}, maximumBlockLoad {0.0f}, callbackMicroseconds {0.0f};
    std::atomic<float> inputPeak {0.0f}, outputPeak {0.0f}, inputBlockRms {0.0f};
    std::atomic<float> rideDb {0.0f}, limiterReductionDb {0.0f};
    std::atomic<float> detectedRmsDb {-160.0f}, requestedRideDb {0.0f};
    std::atomic<bool> riderActive {false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmoothRideAudioProcessor)
};
