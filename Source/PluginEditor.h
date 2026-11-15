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
    void refreshUserPresetList();
    void setNotice(const juce::String& message);

    SmoothRideAudioProcessor& processor;
    static constexpr int numControls = 11;
    std::array<juce::String, numControls> ids {{ "target", "floor", "range", "maxCut", "speed",
                                                   "downSpeed", "mix", "output", "ceiling", "gateHyst", "lowProtect" }};
    std::array<std::unique_ptr<juce::Slider>, numControls> sliders;
    std::array<std::unique_ptr<juce::Label>, numControls> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numControls> attachments;
    juce::ComboBox presets;
    juce::Label presetLabel;
    juce::ComboBox userPresetList;
    juce::TextEditor presetName;
    juce::TextButton savePreset {"Save"}, loadPreset {"Load"}, deletePreset {"Delete"};
    juce::TextButton captureA {"Store A"}, captureB {"Store B"};
    juce::TextButton recallA {"Recall A"}, recallB {"Recall B"};
    juce::Label notice;
    juce::ToggleButton matchAB {"MATCH A/B"};
    juce::TextButton calibrateA {"Cal A"}, calibrateB {"Cal B"};
    juce::Label comparisonInfo;


    std::array<float, 240> gainHistory {};
    std::array<float, 240> rmsHistory {};
    int historyPosition = 0;
    float displayedInputDb = -90.0f, displayedOutputDb = -90.0f;
    float displayedRideDb = 0.0f, displayedPeakReductionDb = 0.0f;
    float displayedRmsDb = -160.0f, displayedRequestedDb = 0.0f;
    bool displayedActive = false;
    float displayedBlockRmsDb = -90.0f;
    float displayedAverageLoad = 0.0f, displayedPeakLoad = 0.0f, displayedCallbackUs = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmoothRideAudioProcessorEditor)
};
