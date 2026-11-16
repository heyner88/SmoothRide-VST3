#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
const juce::Colour panel (0xff10232b), accent (0xff3de9c7);
const juce::Colour faint (0xff8ba9b5), text (0xffeff8f7);
float ampDb(float a) noexcept {
    return juce::Decibels::gainToDecibels(juce::jmax(0.0f, a), -90.0f);
}
 }

SmoothRideSkin::SmoothRideSkin() {
    setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff142a33));
    setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff385760));
    setColour(juce::ComboBox::textColourId,juce::Colour(0xffeff8f7));
    setColour(juce::ComboBox::arrowColourId,juce::Colour(0xff3de9c7));
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff10232b));
    setColour(juce::PopupMenu::textColourId,juce::Colour(0xffeff8f7));
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff24525b));
    setColour(juce::ToggleButton::tickColourId,juce::Colour(0xff3de9c7));
}
void SmoothRideSkin::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
                 float pos,float start,float end,juce::Slider&) {
    const auto center=juce::Point<float>(x+w*0.5f,y+h*0.46f);
    const float radius=juce::jmin((float)w*0.37f,(float)h*0.36f);
    const float a=start+pos*(end-start);
    g.setColour(juce::Colour(0xff041015));
    g.fillEllipse(center.x-radius-2.f,center.y-radius-2.f,2.f*radius+4.f,2.f*radius+4.f);
    juce::Path ring;
    ring.addCentredArc(center.x,center.y,radius,radius,0.f,start,end,true);
    g.setColour(juce::Colour(0xff32535b));
    g.strokePath(ring,juce::PathStrokeType(4.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    juce::Path lit;
    lit.addCentredArc(center.x,center.y,radius,radius,0.f,start,a,true);
    g.setColour(juce::Colour(0xff3de9c7));
    g.strokePath(lit,juce::PathStrokeType(4.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    const float inner=radius-6.f;
    juce::ColourGradient grad(juce::Colour(0xff35525c),center.x-inner,center.y-inner,
                              juce::Colour(0xff0d222b),center.x+inner,center.y+inner,false);
    g.setGradientFill(grad);
    g.fillEllipse(center.x-inner,center.y-inner,inner*2.f,inner*2.f);
    // JUCE Graphics has no drawArc() method; draw the highlight using Path.
    juce::Path highlightArc;
    const float highlightRadius = inner - 2.f;
    highlightArc.addCentredArc(center.x, center.y, highlightRadius, highlightRadius, 0.f,
                              juce::MathConstants<float>::pi * 1.20f,
                              juce::MathConstants<float>::pi * 1.88f, true);
    g.setColour(juce::Colours::white.withAlpha(0.11f));
    g.strokePath(highlightArc, juce::PathStrokeType(1.25f));
    const auto pointer=juce::Point<float>(std::sin(a),-std::cos(a));
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawLine(center.x+pointer.x*inner*0.30f,center.y+pointer.y*inner*0.30f,
               center.x+pointer.x*inner*0.78f,center.y+pointer.y*inner*0.78f,2.2f);
}
void SmoothRideSkin::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,
                                          bool highlighted,bool down) {
    const auto r=b.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(b.getToggleState()?juce::Colour(0xff155c58):(down?juce::Colour(0xff1d6668):highlighted?juce::Colour(0xff1b3c46):juce::Colour(0xff142a33)));
    g.fillRoundedRectangle(r,6.f);
    g.setColour(b.getToggleState()?juce::Colour(0xff3de9c7):(highlighted?juce::Colour(0xff3de9c7):juce::Colour(0xff395965)));
    g.drawRoundedRectangle(r,6.f,1.f);
}

SmoothRideAudioProcessorEditor::SmoothRideAudioProcessorEditor(SmoothRideAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&skin);
    const std::array<juce::String, numControls> names {{ "TARGET RMS", "FLOOR", "MAX BOOST", "MAX CUT",
         "GAIN UP", "GAIN DOWN", "MIX", "OUTPUT GAIN", "CEILING", "HYSTERESIS", "SOFT FLOOR", "ADAPTIVE" }};
    for (int i = 0; i < numControls; ++i) {
        // Binary parameters have their own clickable switches below.
        if (i == 10 || i == 11) continue;
        sliders[i] = std::make_unique<juce::Slider>();
        auto& slider = *sliders[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 20);
        slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff354555));
        slider.setColour(juce::Slider::thumbColourId, text);
        slider.setColour(juce::Slider::textBoxTextColourId, text);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff102730));
        if (ids[i] == "speed" || ids[i] == "downSpeed") slider.setTextValueSuffix(" ms");
        else if (ids[i] == "lowProtect" || ids[i] == "adaptiveFloor") slider.setTextValueSuffix("");
        else if (ids[i] == "mix") slider.setTextValueSuffix(" %");
        else slider.setTextValueSuffix(" dB");
        addAndMakeVisible(slider);
        labels[i] = std::make_unique<juce::Label>();
        labels[i]->setText(names[i], juce::dontSendNotification);
        labels[i]->setJustificationType(juce::Justification::centred);
        labels[i]->setColour(juce::Label::textColourId, faint);
        labels[i]->setFont(juce::Font(juce::FontOptions(11.0f).withStyle("bold")));
        addAndMakeVisible(*labels[i]);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, ids[i], slider);
    }
    for (auto* button : { &softFloorSwitch, &adaptiveFloorSwitch }) {
        button->setClickingTogglesState(true);
        button->setColour(juce::TextButton::buttonColourId, panel);
        button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff155c58));
        button->setColour(juce::TextButton::textColourOffId, text);
        button->setColour(juce::TextButton::textColourOnId, accent);
        addAndMakeVisible(*button);
    }
    softFloorSwitch.setTooltip("Soft Floor: click to turn OFF or ON (0 / 1)");
    adaptiveFloorSwitch.setTooltip("Adaptive Floor: click to turn OFF or ON (0 / 1)");
    softFloorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "lowProtect", softFloorSwitch);
    adaptiveFloorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "adaptiveFloor", adaptiveFloorSwitch);
    presetLabel.setText("FACTORY", juce::dontSendNotification);
    presetLabel.setColour(juce::Label::textColourId, faint);
    presetLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    addAndMakeVisible(presetLabel);
    presets.addItem("Select factory preset", 1);
    presets.addItem("v0.2 Baseline", 2);
    presets.addItem("Music - subtle", 3);
    presets.addItem("Streams - controlled", 4);
    presets.addItem("Movies - wider dynamics", 5);
    presets.setTooltip("Factory presets apply immediately. Saved user presets require Load.");
    presets.setSelectedId(1, juce::dontSendNotification);
    if (processor.isActivePresetFactory()) {
        const auto loaded = processor.getActivePresetName();
        for (int id = 2; id <= 5; ++id) {
            if (presets.getItemText(id - 1) == loaded)
                presets.setSelectedId(id, juce::dontSendNotification);
        }
    }
    presets.onChange = [this] {
        const int selected = presets.getSelectedId();
        if (selected >= 2) applyPreset(selected);
    };
    addAndMakeVisible(presets);

    presetName.setTextToShowWhenEmpty("Preset name (max 40 characters)", faint);
    presetName.setColour(juce::TextEditor::backgroundColourId, panel);
    presetName.setColour(juce::TextEditor::textColourId, text);
    presetName.setColour(juce::TextEditor::outlineColourId, faint.withAlpha(0.35f));
    presetName.setInputRestrictions(40);
    addAndMakeVisible(presetName);
    addAndMakeVisible(userPresetList);
    userPresetList.setTextWhenNothingSelected("Select a saved preset");
    userPresetList.setTooltip("Selecting a user preset does not load it. Click Load to apply.");
    userPresetList.onChange = [this] {
        const auto name = userPresetList.getText();
        if (name.isNotEmpty()) presetName.setText(name, false);
    };
    refreshUserPresetList();
    if (!processor.isActivePresetFactory()) {
        const auto loaded = processor.getActivePresetName();
        const int idx = processor.listUserPresets().indexOf(loaded);
        if (idx >= 0) {
            userPresetList.setSelectedId(idx + 1, juce::dontSendNotification);
            presetName.setText(loaded, false);
        }
    }

    for (auto* button : {&savePreset, &loadPreset, &deletePreset, &updatePreset, &revertPreset,
                         &captureA, &captureB, &recallA, &recallB, &importPreset, &exportPreset}) {
        button->setColour(juce::TextButton::buttonColourId, panel);
        button->setColour(juce::TextButton::textColourOffId, text);
        addAndMakeVisible(*button);
    }
    savePreset.setTooltip("Save as a new named preset; existing presets are never overwritten");
    loadPreset.setTooltip("Load the selected user preset");
    deletePreset.setTooltip("Permanently delete the selected user preset from this plugin state");
    savePreset.onClick = [this] {
        const auto name = presetName.getText().trim();
        if (processor.saveUserPreset(name)) {
            refreshUserPresetList();
            const auto names = processor.listUserPresets();
            const int idx = names.indexOf(name);
            if (idx >= 0) userPresetList.setSelectedId(idx + 1, juce::dontSendNotification);
            setNotice("Saved: " + name);
        } else setNotice("Name exists or invalid - use Update for active preset");
    };
    updatePreset.setTooltip("Overwrite ONLY the active user preset with the current settings");
    revertPreset.setTooltip("Restore the active preset's saved settings; discards unsaved changes");
    updatePreset.onClick = [this] {
        const auto name = processor.getActivePresetName();
        setNotice(processor.updateActiveUserPreset() ? "Updated: " + name
                  : "Load a user preset before updating");
        updatePreset.setEnabled(processor.canUpdateActiveUserPreset() && processor.isActivePresetModified());
        revertPreset.setEnabled(processor.isActivePresetModified());
    };
    revertPreset.onClick = [this] {
        const auto name = processor.getActivePresetName();
        setNotice(processor.revertActivePreset() ? "Reverted: " + name
                  : "No active preset to revert");
        updatePreset.setEnabled(processor.canUpdateActiveUserPreset() && processor.isActivePresetModified());
        revertPreset.setEnabled(processor.isActivePresetModified());
    };
    loadPreset.onClick = [this] {
        const auto name = userPresetList.getText();
        setNotice(processor.loadUserPreset(name) ? "Loaded: " + name : "Select a saved preset");
    };
    deletePreset.onClick = [this] {
        const auto name = userPresetList.getText();
        if (processor.deleteUserPreset(name)) {
            refreshUserPresetList();
            setNotice("Deleted: " + name);
        } else setNotice("Select a saved preset");
    };
    exportPreset.onClick = [this] {
        const auto selected = userPresetList.getText();
        if (selected.isEmpty()) { setNotice("Select a preset to export"); return; }
        const auto initial = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                .getChildFile(juce::File::createLegalFileName(selected) + ".smoothridepreset");
        presetFileChooser = std::make_unique<juce::FileChooser>("Export SmoothRide preset", initial,
                                                                 "*.smoothridepreset");
        juce::Component::SafePointer<SmoothRideAudioProcessorEditor> safe(this);
        presetFileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                      | juce::FileBrowserComponent::canSelectFiles
                                      | juce::FileBrowserComponent::warnAboutOverwriting,
          [safe, selected](const juce::FileChooser& chooser) {
            if (safe == nullptr) return;
            auto file = chooser.getResult();
            if (file == juce::File()) return;
            if (!file.hasFileExtension(".smoothridepreset")) file = file.withFileExtension(".smoothridepreset");
            juce::String error;
            safe->setNotice(safe->processor.exportUserPreset(selected, file, error)
                            ? "Exported: " + selected : error);
          });
    };
    importPreset.onClick = [this] {
        presetFileChooser = std::make_unique<juce::FileChooser>("Import SmoothRide preset",
                        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                        "*.smoothridepreset");
        juce::Component::SafePointer<SmoothRideAudioProcessorEditor> safe(this);
        presetFileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                      | juce::FileBrowserComponent::canSelectFiles,
          [safe](const juce::FileChooser& chooser) {
            if (safe == nullptr) return;
            const auto file = chooser.getResult();
            if (file == juce::File()) return;
            juce::String error;
            if (safe->processor.importUserPreset(file, error)) {
                safe->refreshUserPresetList();
                safe->setNotice("Preset imported");
            } else safe->setNotice(error);
          });
    };
    captureA.onClick = [this] { processor.captureAB(false); setNotice("Stored snapshot A"); };
    captureB.onClick = [this] { processor.captureAB(true); setNotice("Stored snapshot B"); };
    recallA.onClick = [this] {
        setNotice(processor.recallAB(false) ? "Recalled A" : "Store A first");
    };
    recallB.onClick = [this] {
        setNotice(processor.recallAB(true) ? "Recalled B" : "Store B first");
    };
    matchAB.setToggleState(processor.isABMatching(), juce::dontSendNotification);
    matchAB.onClick = [this] {
        processor.setABMatching(matchAB.getToggleState());
        setNotice(matchAB.getToggleState() ? "Match enabled: calibrate both slots" : "Match disabled");
    };
    addAndMakeVisible(matchAB);
    for (auto* button : {&calibrateA, &calibrateB}) {
        button->setColour(juce::TextButton::buttonColourId, panel);
        button->setColour(juce::TextButton::textColourOffId, text);
        addAndMakeVisible(*button);
    }
    calibrateA.onClick = [this] {
        setNotice(processor.calibrateAB(false) ? "A measured. Recall slots after both calibrated." : "Store A first");
    };
    calibrateB.onClick = [this] {
        setNotice(processor.calibrateAB(true) ? "B measured. Recall slots after both calibrated." : "Store B first");
    };
    comparisonInfo.setColour(juce::Label::textColourId, faint);
    comparisonInfo.setFont(juce::Font(juce::FontOptions(11.f)));
    addAndMakeVisible(comparisonInfo);
    noiseGuard.setColour(juce::ToggleButton::textColourId, faint);
    noiseGuardAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "noiseGuard", noiseGuard);
    addAndMakeVisible(noiseGuard);
    notice.setColour(juce::Label::textColourId, faint);
    notice.setJustificationType(juce::Justification::centredRight);
    notice.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(notice);
    resetDSP.setColour(juce::TextButton::buttonColourId, panel);
    resetDSP.setColour(juce::TextButton::textColourOffId, text);
    resetDSP.onClick = [this] {
        processor.requestDSPReset();
        setNotice("DSP reset requested; updates on next audio block");
    };
    addAndMakeVisible(resetDSP);
    detailDSP.setColour(juce::ToggleButton::textColourId, faint);
    detailDSP.setToggleState(processor.getDetailedDSPDiagnostics(), juce::dontSendNotification);
    detailDSP.onClick = [this] {
        processor.setDetailedDSPDiagnostics(detailDSP.getToggleState());
        processor.requestDSPReset(); // Never mix detailed and lightweight measurements.
        setNotice(detailDSP.getToggleState() ? "Detailed timing ON; DSP statistics reset" : "Lightweight timing ON; DSP statistics reset");
    };
    addAndMakeVisible(detailDSP);
    spikeThreshold.addItem("5%", 1);
    spikeThreshold.addItem("10%", 2);
    spikeThreshold.addItem("20%", 3);
    spikeThreshold.addItem("40%", 4);
    const float initialThreshold = processor.getDSPSpikeThreshold();
    spikeThreshold.setSelectedId(initialThreshold == 5.f ? 1 : initialThreshold == 20.f ? 3 : initialThreshold == 40.f ? 4 : 2, juce::dontSendNotification);
    spikeThreshold.onChange = [this] {
        const float values[] = {5.f, 10.f, 20.f, 40.f};
        const auto id = spikeThreshold.getSelectedId();
        if (id >= 1 && id <= 4) processor.setDSPSpikeThreshold(values[id-1]);
    };
    addAndMakeVisible(spikeThreshold);
    listenView.setColour(juce::ToggleButton::textColourId, accent);
    listenView.setToggleState(true, juce::dontSendNotification);
    listenView.onClick = [this] {
        updateView();
        setNotice(listenView.getToggleState() ? "Listening view" : "Advanced view");
    };
    addAndMakeVisible(listenView);
    updateView();
    gainHistory.fill(0.0f);
    rmsHistory.fill(-100.0f);
    // Fixed compact editor: never resize host-owned VST3 windows interactively.
    setResizable(false, false);
    setSize(1080, 600);
    startTimerHz(10);
}

SmoothRideAudioProcessorEditor::~SmoothRideAudioProcessorEditor() { stopTimer(); setLookAndFeel(nullptr); }

void SmoothRideAudioProcessorEditor::updateView() {
    const bool advanced = !listenView.getToggleState();
    detailDSP.setVisible(advanced);
    resetDSP.setVisible(advanced);
    spikeThreshold.setVisible(advanced);
    // Listening mode keeps the actions used most frequently. Advanced mode
    // retains every existing action, including portable preset operations.
    for (auto* button : { &deletePreset, &importPreset, &exportPreset,
                          &captureA, &captureB, &calibrateA, &calibrateB })
        button->setVisible(advanced);
    // Keep operation feedback visible in both modes; its bounds depend on the mode.
    notice.setVisible(true);
    comparisonInfo.setVisible(false);
    resized();
    // Only controls visibility. Existing parameter values and DSP telemetry remain untouched.
    repaint();
}

void SmoothRideAudioProcessorEditor::timerCallback() {
    const bool modified = processor.isActivePresetModified();
    // Reflect APVTS automation, preset recall and mouse clicks in the button text.
    softFloorSwitch.setButtonText(softFloorSwitch.getToggleState() ? "SOFT FLOOR   ON" : "SOFT FLOOR   OFF");
    adaptiveFloorSwitch.setButtonText(adaptiveFloorSwitch.getToggleState() ? "ADAPTIVE   ON" : "ADAPTIVE   OFF");
    updatePreset.setEnabled(modified && processor.canUpdateActiveUserPreset());
    revertPreset.setEnabled(modified);
    // Factory selector reflects the actual loaded factory preset, not an earlier choice.
    int factorySelection = 1;
    if (processor.isActivePresetFactory()) {
        const auto factoryName = processor.getActivePresetName();
        for (int id = 2; id <= 5; ++id)
            if (presets.getItemText(id - 1) == factoryName) factorySelection = id;
    }
    if (presets.getSelectedId() != factorySelection)
        presets.setSelectedId(factorySelection, juce::dontSendNotification);
    displayedInputDb = ampDb(processor.getInputPeak());
    displayedOutputDb = ampDb(processor.getOutputPeak());
    displayedInputLDb = ampDb(processor.getInputChannelPeak(0));
    displayedInputRDb = ampDb(processor.getInputChannelPeak(1));
    displayedOutputLDb = ampDb(processor.getOutputChannelPeak(0));
    displayedOutputRDb = ampDb(processor.getOutputChannelPeak(1));
    const std::array<float, 4> peakValues {{ displayedInputLDb, displayedInputRDb,
                                            displayedOutputLDb, displayedOutputRDb }};
    for (size_t channel = 0; channel < heldPeakDb.size(); ++channel) {
        if (peakValues[channel] >= heldPeakDb[channel]) {
            heldPeakDb[channel] = peakValues[channel];
            peakHoldTicks[channel] = 12; // Hold for ~1.2 seconds at 10 Hz.
        } else if (peakHoldTicks[channel] > 0) {
            --peakHoldTicks[channel];
        } else {
            heldPeakDb[channel] = juce::jmax(peakValues[channel], heldPeakDb[channel] - 2.0f);
        }
    }
    displayedRideDb = processor.getRideDb();
    displayedPeakReductionDb = processor.getLimiterReductionDb();
    displayedRmsDb = processor.getDetectedRmsDb();
    displayedBlockRmsDb = ampDb(processor.getInputBlockRms());
    displayedRequestedDb = processor.getRequestedRideDb();
    displayedActive = processor.getRiderActive();
    const auto dsp = processor.getDSPSnapshot();
    processor.rememberDSPSnapshot(dsp);
    displayedAverageLoad = dsp.average;
    displayedPeakLoad = dsp.historicalPeakEvent.block; // Exact same callback as PEAK stages
    displayedRollingPeak = dsp.rollingPeak;
    displayedSpikes = dsp.spikes;
    displayedCallbackUs = dsp.callbackUs;
    displayedLastSpikePercent = dsp.lastSpikePercent;
    displayedLastSpikeUs = dsp.lastSpikeUs;
    displayedLastSpikeBudgetUs = dsp.lastSpikeBudgetUs;
    displayedGuardToggles = dsp.guardToggles;
    displayedLastTogglePercent = dsp.lastTogglePercent;
    displayedLastToggleRiderPercent = dsp.lastToggleRiderPercent;
    displayedLastTogglePrePercent = dsp.lastTogglePrePercent;
    displayedLastTogglePostPercent = dsp.lastTogglePostPercent;
    displayedMonitorUs = dsp.monitorUs;
    displayedToggleParams = dsp.lastToggleParamsPercent;
    displayedToggleInput = dsp.lastToggleInputPercent;
    displayedToggleAB = dsp.lastToggleABPercent;
    displayedNormalPre = dsp.normalPreAveragePercent;
    displayedNormalParams = dsp.normalParamsPercent;
    displayedToggleParamsMean = dsp.toggleParamsMeanPercent;
    displayedToggleParamsMax = dsp.toggleParamsMaxPercent;
    displayedResetUs = dsp.resetMicroseconds;
    displayedPeakEvent = dsp.historicalPeakEvent;
    displayedSpikeEvent = dsp.lastSpikeEvent;
    displayedToggleSpikes = dsp.toggleSpikes;
    displayedNormalSpikes = dsp.normalSpikes;
    displayedABRunning = dsp.lastToggleABRunning;
    comparisonInfo.setText("Output RMS " + juce::String(processor.getRecentOutputRmsDb(), 1)
       + " dBFS   |   Match " + juce::String(processor.getABMatchingDb(), 1) + " dB"
       + "   |   Floor " + juce::String(processor.getEffectiveFloorDb(), 1) + " dB",
       juce::dontSendNotification);
    gainHistory[(size_t)historyPosition] = displayedRideDb;
    rmsHistory[(size_t)historyPosition] = displayedRmsDb;
    historyPosition = (historyPosition + 1) % (int)gainHistory.size();
    repaint(juce::Rectangle<int>(14, 0, getWidth()-28, 306));
    // Active preset state can also change through host automation or preset recall.
    repaint(juce::Rectangle<int>(388, 482, 530, 29));
}

void SmoothRideAudioProcessorEditor::refreshUserPresetList() {
    auto previous = userPresetList.getText();
    userPresetList.clear(juce::dontSendNotification);
    const auto names = processor.listUserPresets();
    for (int i = 0; i < names.size(); ++i) userPresetList.addItem(names[i], i+1);
    const auto index = names.indexOf(previous);
    if (index >= 0) userPresetList.setSelectedId(index+1, juce::dontSendNotification);
}

void SmoothRideAudioProcessorEditor::setNotice(const juce::String& message) {
    notice.setText(message, juce::dontSendNotification);
    notice.setTooltip(message);
}

void SmoothRideAudioProcessorEditor::applyPreset(int presetId) {
    // Optional parameter changes on the UI thread, never the audio thread.
    // The baseline exactly matches the v0.2 defaults; all presets retain safe limited riding.
    const std::array<float, 12> baseline {{-12.f, -60.f, 6.f, 3.f, 3000.f, 650.f, 65.f, 6.f, -1.f, 0.f, 0.f, 0.f}};
    const std::array<float, 12> music    {{-12.f, -60.f, 4.f, 2.f, 4000.f, 900.f, 45.f, 6.f, -1.f, 0.f, 0.f, 0.f}};
    const std::array<float, 12> streams  {{-12.f, -60.f, 6.f, 3.f, 3000.f, 650.f, 65.f, 6.f, -1.f, 0.f, 0.f, 0.f}};
    const std::array<float, 12> movies   {{-14.f, -60.f, 3.f, 2.f, 5000.f, 1100.f, 40.f, 4.f, -1.f, 0.f, 0.f, 0.f}};
    const auto& values = presetId == 3 ? music : presetId == 4 ? streams : presetId == 5 ? movies : baseline;
    for (int i = 0; i < numControls; ++i) {
        if (auto* param = processor.apvts.getParameter(ids[i])) {
            const float norm = param->convertTo0to1(values[(size_t)i]);
            param->beginChangeGesture();
            param->setValueNotifyingHost(norm);
            param->endChangeGesture();
        }
    }
    if (presetId >= 2 && presetId <= 5)
        processor.markActivePreset(presets.getItemText(presetId - 1), true);
}

void SmoothRideAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> r,
                                               float leftDb, float rightDb, const juce::String& caption) const {
    const auto muted = juce::Colour(0xff8ba9b5);
    const auto mint = juce::Colour(0xff3de9c7);
    const auto amber = juce::Colour(0xffffc36c);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("bold")));
    g.drawText(caption, r.removeFromTop(22).toNearestInt(), juce::Justification::centred);
    const float top = r.getY() + 11.f, bottom = r.getBottom() - 38.f;
    const float trackH = bottom - top;
    const float trackWidth = 23.f;
    const float xL = r.getCentreX() - 28.f, xR = r.getCentreX() + 5.f;
    const size_t offset = caption.startsWithIgnoreCase("INPUT") ? 0u : 2u;
    for (int channel = 0; channel < 2; ++channel) {
        const float x = channel == 0 ? xL : xR;
        const float db = channel == 0 ? leftDb : rightDb;
        const auto track = juce::Rectangle<float>(x, top, trackWidth, trackH);
        g.setColour(juce::Colour(0xff061218));
        g.fillRoundedRectangle(track, 4.f);
        const float fraction = juce::jlimit(0.f, 1.f, (db + 60.f)/60.f);
        auto fill = track.reduced(3.f);
        fill.setY(fill.getBottom() - fill.getHeight()*fraction);
        fill.setHeight((trackH-6.f)*fraction);
        juce::ColourGradient gradient(amber, x+11.f, top, mint, x+11.f, bottom, false);
        g.setGradientFill(gradient);
        if (fill.getHeight() > .5f) g.fillRoundedRectangle(fill, 2.f);
        // Thin amber peak-hold line, separate for each real stereo channel.
        const float holdDb = heldPeakDb[offset + static_cast<size_t>(channel)];
        if (holdDb > -60.f) {
            const float heldFraction = juce::jlimit(0.f, 1.f, (holdDb + 60.f) / 60.f);
            const float markerY = bottom - 3.f - heldFraction * (trackH - 6.f);
            g.setColour(amber.withAlpha(0.95f));
            g.drawHorizontalLine(static_cast<int>(markerY), x + 3.f, x + trackWidth - 3.f);
        }
        g.setColour(juce::Colour(0xff41606b));
        g.drawRoundedRectangle(track, 4.f, 1.f);
        g.setColour(muted);
        g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText(channel == 0 ? "L" : "R", (int)x-1, (int)bottom+4, (int)trackWidth+2, 14,
                   juce::Justification::centred);
    }
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(9.f)));
    for (int db=0; db>=-60; db-=12) {
        const float y = top + (float)(-db)/60.f*trackH;
        g.drawText(juce::String(db), (int)r.getX()-1, (int)y-7, 24, 14, juce::Justification::centredRight);
    }
    // Independent numeric channel readings: no fabricated peak-hold values.
    g.setColour(juce::Colour(0xffeaf7f5));
    g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("bold")));
    const auto leftText = juce::String(leftDb, 1);
    const auto rightText = juce::String(rightDb, 1);
    g.drawFittedText("L " + leftText + "   R " + rightText,
                     (int)r.getX(), (int)bottom+20, (int)r.getWidth(), 18,
                     juce::Justification::centred, 1, 0.8f);
}

void SmoothRideAudioProcessorEditor::paint(juce::Graphics& g) {
    const bool listening = listenView.getToggleState();
    const juce::Colour background(0xff09141b), surface(0xff10232b), edge(0xff294b54);
    const juce::Colour mint(0xff3de9c7), pale(0xffeff8f7), muted(0xff8ba9b5), amber(0xffffc36c);
    g.fillAll(background);
    juce::ColourGradient topGlow(juce::Colour(0xff163a40), 540.f, -80.f, background, 540.f, 270.f, false);
    g.setGradientFill(topGlow);
    g.fillRect(0, 0, getWidth(), 300);
    const auto roundedPanel=[&](juce::Rectangle<float> r, float rad=11.f) {
        g.setColour(surface);
        g.fillRoundedRectangle(r, rad);
        g.setColour(edge);
        g.drawRoundedRectangle(r, rad, 1.f);
    };
    const auto title=[&](const juce::String& name, int x, int y, int width, juce::Colour colour=juce::Colour(0xffeff8f7)) {
        g.setColour(colour);
        g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("bold")));
        g.drawText(name, x,y,width,20,juce::Justification::centredLeft);
    };
    // Brand and application mode.
    g.setColour(mint);
    juce::Path logo;
    logo.startNewSubPath(18.f,23.f); logo.lineTo(27.f,23.f); logo.lineTo(32.f,14.f);
    logo.lineTo(40.f,36.f); logo.lineTo(48.f,9.f); logo.lineTo(56.f,23.f); logo.lineTo(71.f,23.f);
    g.strokePath(logo, juce::PathStrokeType(2.5f));
    g.setColour(pale);
    g.setFont(juce::Font(juce::FontOptions(27.f).withStyle("bold")));
    g.drawText("Smooth", 81, 6, 108, 39, juce::Justification::centredLeft);
    g.setColour(mint);
    g.drawText("Ride", 184, 6, 76, 39, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.drawText("INTELLIGENT GAIN RIDER", 270, 15, 222, 22, juce::Justification::centredLeft);
    g.setColour(edge);
    g.drawHorizontalLine(49, 14.f, 1067.f);
    g.setColour(mint);
    g.drawText("v1.0 RC1", 970, 14, 94, 23, juce::Justification::centredRight);

    // Tall meter wells, plus an expanded central monitoring surface.
    roundedPanel({14.f,58.f,116.f,240.f});
    roundedPanel({950.f,58.f,116.f,240.f});
    drawMeter(g, {20.f,63.f,104.f,229.f},displayedInputLDb,displayedInputRDb,"INPUT PEAK");
    drawMeter(g, {956.f,63.f,104.f,229.f},displayedOutputLDb,displayedOutputRDb,"OUTPUT PEAK");
    roundedPanel({140.f,58.f,800.f,240.f});
    title(listening ? "GAIN RIDING  /  LIVE HISTORY" : "ADVANCED DSP  /  CALLBACK TELEMETRY",155,68,410,mint);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.f)));
    g.drawText("AVG " + juce::String(displayedAverageLoad,2) + "%  |  PEAK "
              + juce::String(displayedPeakLoad,2) + "%  |  60s "
              + juce::String(displayedRollingPeak,2) + "%",585,68,338,20,juce::Justification::centredRight);

    if (listening) {
        const auto graph=juce::Rectangle<float>(164.f,104.f,751.f,116.f);
        g.setColour(juce::Colour(0xff091a21));
        g.fillRoundedRectangle(graph,5.f);
        for (int i=0;i<=4;++i) {
            float y=graph.getY()+graph.getHeight()*i/4.f;
            g.setColour(juce::Colour(0xff27414c).withAlpha(0.62f));
            g.drawHorizontalLine((int)y,graph.getX(),graph.getRight());
        }
        for (int i=0;i<=10;++i) {
            float x=graph.getX()+graph.getWidth()*i/10.f;
            g.setColour(juce::Colour(0xff27414c).withAlpha(0.30f));
            g.drawVerticalLine((int)x,graph.getY(),graph.getBottom());
        }
        g.setColour(juce::Colour(0xff57727c));
        g.drawHorizontalLine((int)graph.getCentreY(),graph.getX(),graph.getRight());
        juce::Path ride, rms;
        for (size_t i=0;i<gainHistory.size();++i) {
            const size_t h=(size_t)((historyPosition+(int)i)%(int)gainHistory.size());
            const float x=graph.getX()+(float)i/(float)(gainHistory.size()-1)*graph.getWidth();
            const float yy=graph.getCentreY()-juce::jlimit(-12.f,12.f,gainHistory[h])/12.f*graph.getHeight()/2.f;
            const float norm=juce::jlimit(0.f,1.f,(rmsHistory[h]+100.f)/100.f);
            const float yr=graph.getBottom()-norm*graph.getHeight();
            if (i==0) {ride.startNewSubPath(x,yy);rms.startNewSubPath(x,yr);}
            else {ride.lineTo(x,yy);rms.lineTo(x,yr);}
        }
        g.setColour(mint);
        g.strokePath(ride,juce::PathStrokeType(2.1f));
        g.setColour(juce::Colour(0xff64aaff));
        g.strokePath(rms,juce::PathStrokeType(1.35f));
    } else {
        const auto line=[&](const juce::String& t, int y, juce::Colour col=juce::Colour(0xff8ba9b5)) {
            g.setColour(col);
            g.setFont(juce::Font(juce::FontOptions(y == 101 ? 12.f : 11.f)
                .withStyle(y == 101 ? "bold" : "Regular")));
            g.drawFittedText(t,161,y,756,20,juce::Justification::centredLeft,1);
        };
        const auto event=[&](const juce::String& name,const smoothride::SpikeEvent& e) {
            return name+"  #"+juce::String(static_cast<juce::int64>(e.callbackId))
                  +"  BLOCK "+juce::String(e.block,2)+"%  P "+juce::String(e.params,1)
                  +"  I "+juce::String(e.input,1)+"  AB "+juce::String(e.ab,1)
                  +"  R "+juce::String(e.rider,1)+"  O "+juce::String(e.output,1)
                  +"  "+(e.guardToggle?"TOGGLE":"NORMAL");
        };
        line("LAST "+juce::String(displayedCallbackUs,1)+" us   |   SPIKES "
           +juce::String(static_cast<juce::int64>(displayedSpikes))
           +"   |   GUARD CHANGES "+juce::String(static_cast<juce::int64>(displayedGuardToggles)),101,pale);
        g.setColour(juce::Colour(0xff213c45));
        g.drawHorizontalLine(125, 158.f, 921.f);
        line(event("PEAK",displayedPeakEvent),128,juce::Colour(0xffe5eeed));
        line(event("SPIKE",displayedSpikeEvent),153);
        line("BASE PARAMS "+juce::String(displayedNormalParams,3)+"%  |  TOGGLE AVG/MAX "
             +juce::String(displayedToggleParamsMean,2)+" / "
             +juce::String(displayedToggleParamsMax,2)+"%  |  RESET "
             +juce::String(displayedResetUs,1)+" us",179);
        line("THRESHOLD SPIKES: TOGGLE "
             +juce::String(static_cast<juce::int64>(displayedToggleSpikes))
             +"   NORMAL "+juce::String(static_cast<juce::int64>(displayedNormalSpikes))
             +"  |  DETAIL DSP " + (detailDSP.getToggleState()?"ON":"OFF"),205,mint);
    }
    // Three foreground values below the live chart or diagnostic grid.
    g.setColour(juce::Colour(0xff213c45));
    g.drawHorizontalLine(235,160.f,923.f);
    const auto statistic=[&](const juce::String& name, const juce::String& value, int x, int w,juce::Colour color) {
        g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
        g.drawText(name,x,240,w,17,juce::Justification::centred);
        g.setColour(color);g.setFont(juce::Font(juce::FontOptions(name == "GAIN RIDE" || name == "RIDER STATUS" ? 24.f : 19.f).withStyle("bold")));
        g.drawText(value,x,257,w,31,juce::Justification::centred);
    };
    statistic("DETECTOR RMS",juce::String(displayedRmsDb,1)+" dBFS",155,228,pale);
    statistic("GAIN RIDE",(displayedRideDb>=0?"+":"")+juce::String(displayedRideDb,1)+" dB",388,228,mint);
    const float boost=processor.apvts.getRawParameterValue("range")->load();
    const float cut=processor.apvts.getRawParameterValue("maxCut")->load();
    const juce::String status=!displayedActive?"BELOW FLOOR":displayedRequestedDb>=boost-0.05f?"MAX BOOST":displayedRequestedDb<=-cut+0.05f?"MAX CUT":"ACTIVE";
    statistic("RIDER STATUS",processor.getNoiseBlocked()?"NOISE BLOCKED":status,625,300,displayedActive?mint:muted);

    // Four functional control groups replace the old uniform parameter grid.
    struct Group {int x,w;const char* name;};
    const Group groups[]={{14,250,"LEVELING"},{272,250,"RESPONSE"},{530,318,"DETECTION"},{856,210,"OUTPUT"}};
    for (const auto& group:groups) {
        roundedPanel({(float)group.x,306.f,(float)group.w,166.f},10.f);
        title(group.name,group.x+13,313,group.w-20,mint);
        g.setColour(juce::Colour(0xff2d5058));
        g.drawHorizontalLine(337,(float)group.x+8,(float)group.x+group.w-8);
    }
    const auto activePreset=processor.getActivePresetName();
    const bool modified=processor.isActivePresetModified();
    const juce::Colour stateColour=modified?amber:activePreset.isEmpty()?muted:mint;
    g.setColour(juce::Colour(0xff0f252d));
    g.fillRoundedRectangle(juce::Rectangle<float>(14.f,480.f,1052.f,112.f),10.f);
    g.setColour(edge);
    g.drawRoundedRectangle(juce::Rectangle<float>(14.f,480.f,1052.f,112.f),10.f,1.f);
    g.setFont(juce::Font(juce::FontOptions(10.f).withStyle("bold")));
    g.setColour(muted);
    // Discrete status dot and a concise state label, without covering preset controls.
    g.setColour(stateColour);
    g.fillEllipse(391.f, 493.f, 6.f, 6.f);
    const juce::String state=modified?"MODIFIED":activePreset.isEmpty()?"CUSTOM":"SAVED";
    g.drawText(state,403,487,76,19,juce::Justification::centredLeft);
    g.setColour(pale);
    g.setFont(juce::Font(juce::FontOptions(11.f)));
    g.drawFittedText(activePreset.isEmpty()?"No active preset":"ACTIVE: "+activePreset,
                     482,487,420,20,juce::Justification::centredLeft,1);

}

void SmoothRideAudioProcessorEditor::resized() {
    struct Slot {int i,x,w;};
    const Slot slots[]={{0,23,75},{2,105,75},{3,187,75},
                        {4,279,75},{5,361,75},{6,443,75},
                        {1,539,70},{9,614,70},
                        {7,863,94},{8,963,94}};
    for (const auto& item:slots) {
        if (labels[(size_t)item.i] != nullptr && sliders[(size_t)item.i] != nullptr) {
            labels[(size_t)item.i]->setBounds(item.x,427,item.w,19);
            sliders[(size_t)item.i]->setBounds(item.x,346,item.w,82);
        }
    }
    // Detection safety controls live in their own reserved footer, not over a knob.
    softFloorSwitch.setBounds(685,350,150,32);
    adaptiveFloorSwitch.setBounds(685,390,150,32);
    noiseGuard.setBounds(566,445,265,24);
    const bool listening = listenView.getToggleState();
    presetLabel.setBounds(23,483,76,27);
    presets.setBounds(104,483,278,27);
    listenView.setBounds(921,483,137,26);
    if (listening) {
        userPresetList.setBounds(22,519,260,28);
        presetName.setBounds(290,519,248,28);
        savePreset.setBounds(545,519,72,28);
        loadPreset.setBounds(625,519,72,28);
        updatePreset.setBounds(705,519,80,28);
        revertPreset.setBounds(793,519,80,28);
        recallA.setBounds(22,556,87,27);
        recallB.setBounds(117,556,87,27);
        matchAB.setBounds(218,555,125,28);
        notice.setBounds(354,555,690,28);
    } else {
        userPresetList.setBounds(21,518,205,26);
        presetName.setBounds(233,518,205,26);
        savePreset.setBounds(444,518,65,26);
        loadPreset.setBounds(514,518,61,26);
        updatePreset.setBounds(580,518,75,26);
        revertPreset.setBounds(661,518,75,26);
        deletePreset.setBounds(741,518,65,26);
        importPreset.setBounds(813,518,78,26);
        exportPreset.setBounds(898,518,78,26);
        captureA.setBounds(21,554,75,27);
        captureB.setBounds(101,554,75,27);
        recallA.setBounds(181,554,75,27);
        recallB.setBounds(261,554,75,27);
        matchAB.setBounds(350,554,116,27);
        calibrateA.setBounds(475,554,66,27);
        calibrateB.setBounds(546,554,66,27);
        detailDSP.setBounds(614,551,105,29);
        resetDSP.setBounds(724,554,91,27);
        spikeThreshold.setBounds(821,554,90,27);
        // The rightmost 140 px are reserved for truncated feedback;
        // hovering reveals the complete notification.
        notice.setBounds(917,552,137,31);
    }
}
