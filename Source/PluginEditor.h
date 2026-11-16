#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

// UI-only skin: no DSP allocations or processing are added to the audio callback.
class SmoothRideSkin final : public juce::LookAndFeel_V4 {
public:
    SmoothRideSkin();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool highlighted, bool down) override;
};

class SmoothRideAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer {
public:
    explicit SmoothRideAudioProcessorEditor(SmoothRideAudioProcessor&);
    ~SmoothRideAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void drawMeter(juce::Graphics&, juce::Rectangle<float>, float leftDb, float rightDb,
                   const juce::String& caption) const;
    void applyPreset(int preset);
    void refreshUserPresetList();
    void setNotice(const juce::String& message);
    void updateView();

    SmoothRideAudioProcessor& processor;
    SmoothRideSkin skin;
    static constexpr int numControls = 12;
    std::array<juce::String, numControls> ids {{ "target", "floor", "range", "maxCut", "speed",
                                                   "downSpeed", "mix", "output", "ceiling", "gateHyst", "lowProtect", "adaptiveFloor" }};
    std::array<std::unique_ptr<juce::Slider>, numControls> sliders;
    std::array<std::unique_ptr<juce::Label>, numControls> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numControls> attachments;
    // Two-state parameters use dedicated buttons instead of hard-to-drag rotary sliders.
    juce::TextButton softFloorSwitch {"SOFT FLOOR"}, adaptiveFloorSwitch {"ADAPTIVE FLOOR"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> softFloorAttachment, adaptiveFloorAttachment;
    juce::ComboBox presets;
    juce::Label presetLabel;
    juce::ComboBox userPresetList;
    juce::TextEditor presetName;
    juce::TextButton savePreset {"Save"}, loadPreset {"Load"}, deletePreset {"Delete"};
    juce::TextButton updatePreset {"Update"}, revertPreset {"Revert"};
    juce::TextButton importPreset {"Import"}, exportPreset {"Export"};
    std::unique_ptr<juce::FileChooser> presetFileChooser;
    juce::TextButton captureA {"Store A"}, captureB {"Store B"};
    juce::TextButton recallA {"Recall A"}, recallB {"Recall B"};
    juce::Label notice;
    juce::ToggleButton matchAB {"MATCH A/B"};
    juce::TextButton calibrateA {"Cal A"}, calibrateB {"Cal B"};
    juce::Label comparisonInfo;
    juce::TextButton resetDSP {"Reset DSP"};
    juce::ToggleButton detailDSP {"DETAIL DSP"};
    juce::ToggleButton listenView {"LISTEN VIEW"};
    juce::ComboBox spikeThreshold;

    juce::ToggleButton noiseGuard {"NOISE GUARD (EXP)"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> noiseGuardAttachment;


    std::array<float, 240> gainHistory {};
    std::array<float, 240> rmsHistory {};
    int historyPosition = 0;
    float displayedInputDb = -90.0f, displayedOutputDb = -90.0f;
    float displayedInputLDb = -90.0f, displayedInputRDb = -90.0f;
    float displayedOutputLDb = -90.0f, displayedOutputRDb = -90.0f;
    // UI-only peak-hold markers; no additional work in the audio callback.
    std::array<float, 4> heldPeakDb {{-90.f, -90.f, -90.f, -90.f}};
    std::array<int, 4> peakHoldTicks {{0, 0, 0, 0}};
    float displayedRideDb = 0.0f, displayedPeakReductionDb = 0.0f;
    float displayedRmsDb = -160.0f, displayedRequestedDb = 0.0f;
    bool displayedActive = false;
    float displayedBlockRmsDb = -90.0f;
    float displayedAverageLoad = 0.0f, displayedPeakLoad = 0.0f, displayedCallbackUs = 0.0f;
    float displayedRollingPeak = 0.0f;
    std::uint64_t displayedSpikes = 0, displayedGuardToggles = 0;
    float displayedLastTogglePercent = 0, displayedLastToggleRiderPercent = 0;
    float displayedLastTogglePrePercent = 0, displayedLastTogglePostPercent = 0, displayedMonitorUs = 0;
    float displayedLastSpikePercent = 0, displayedLastSpikeUs = 0, displayedLastSpikeBudgetUs = 0;
    float displayedToggleParams = 0, displayedToggleInput = 0, displayedToggleAB = 0, displayedNormalPre = 0;
    float displayedNormalParams = 0, displayedToggleParamsMean = 0, displayedToggleParamsMax = 0, displayedResetUs = 0;
    smoothride::SpikeEvent displayedPeakEvent {}, displayedSpikeEvent {};
    std::uint64_t displayedToggleSpikes = 0, displayedNormalSpikes = 0;
    bool displayedABRunning = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmoothRideAudioProcessorEditor)
};
