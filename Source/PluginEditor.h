#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class SmoothRideAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer {
public:
    explicit SmoothRideAudioProcessorEditor(SmoothRideAudioProcessor&);
    ~SmoothRideAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void drawMeter(juce::Graphics&, juce::Rectangle<float>, float levelDb,
                   const juce::String& caption) const;
    void applyPreset(int preset);
    SmoothRideAudioProcessor& processor;
    static constexpr int numControls = 9;
    std::array<juce::String, numControls> ids {{ "target", "floor", "range", "maxCut", "speed",
                                                   "downSpeed", "mix", "output", "ceiling" }};
    std::array<std::unique_ptr<juce::Slider>, numControls> sliders;
    std::array<std::unique_ptr<juce::Label>, numControls> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numControls> attachments;
    juce::ComboBox presets;
    juce::Label presetLabel;
    std::array<float, 240> gainHistory {};
    int historyPosition = 0;
    float displayedInputDb = -90.0f, displayedOutputDb = -90.0f;
    float displayedRideDb = 0.0f, displayedPeakReductionDb = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmoothRideAudioProcessorEditor)
};
