#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
const juce::Colour bg (0xff101721), panel (0xff1c2835), accent (0xff54ddc4);
const juce::Colour faint (0xff879aab), text (0xffedf4f8);
float ampDb(float a) noexcept {
    return juce::Decibels::gainToDecibels(juce::jmax(0.0f, a), -90.0f);
}
}

SmoothRideAudioProcessorEditor::SmoothRideAudioProcessorEditor(SmoothRideAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p) {
    const std::array<juce::String, numControls> names {{ "TARGET RMS", "FLOOR", "MAX BOOST", "MAX CUT",
         "GAIN UP", "GAIN DOWN", "MIX", "OUTPUT", "CEILING", "FLOOR HYST", "SOFT FLOOR", "ADAPTIVE FLOOR" }};
    for (int i = 0; i < numControls; ++i) {
        sliders[i] = std::make_unique<juce::Slider>();
        auto& slider = *sliders[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 17);
        slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff354555));
        slider.setColour(juce::Slider::thumbColourId, text);
        slider.setColour(juce::Slider::textBoxTextColourId, text);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, panel);
        if (ids[i] == "speed" || ids[i] == "downSpeed") slider.setTextValueSuffix(" ms");
        else if (ids[i] == "lowProtect" || ids[i] == "adaptiveFloor") slider.setTextValueSuffix(" (0=OFF, 1=ON)");
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
    presetLabel.setText("OPTIONAL STARTING PRESET", juce::dontSendNotification);
    presetLabel.setColour(juce::Label::textColourId, faint);
    presetLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    addAndMakeVisible(presetLabel);
    presets.addItem("Choose a preset (changes parameters)", 1);
    presets.addItem("v0.2 Baseline", 2);
    presets.addItem("Music - subtle", 3);
    presets.addItem("Streams - controlled", 4);
    presets.addItem("Movies - wider dynamics", 5);
    presets.setSelectedId(1, juce::dontSendNotification);
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
    userPresetList.setTextWhenNothingSelected("Saved presets");
    userPresetList.onChange = [this] {
        const auto name = userPresetList.getText();
        if (name.isNotEmpty()) presetName.setText(name, false);
    };
    refreshUserPresetList();

    for (auto* button : {&savePreset, &loadPreset, &deletePreset,
                         &captureA, &captureB, &recallA, &recallB, &importPreset, &exportPreset}) {
        button->setColour(juce::TextButton::buttonColourId, panel);
        button->setColour(juce::TextButton::textColourOffId, text);
        addAndMakeVisible(*button);
    }
    savePreset.onClick = [this] {
        const auto name = presetName.getText().trim();
        if (processor.saveUserPreset(name)) {
            refreshUserPresetList();
            const auto names = processor.listUserPresets();
            const int idx = names.indexOf(name);
            if (idx >= 0) userPresetList.setSelectedId(idx + 1, juce::dontSendNotification);
            setNotice("Saved: " + name);
        } else setNotice("Enter a name up to 40 characters");
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
    gainHistory.fill(0.0f);
    rmsHistory.fill(-100.0f);
    // Fixed compact editor: never resize host-owned VST3 windows interactively.
    setResizable(false, false);
    setSize(1080, 600);
    startTimerHz(10);
}

SmoothRideAudioProcessorEditor::~SmoothRideAudioProcessorEditor() { stopTimer(); }

void SmoothRideAudioProcessorEditor::timerCallback() {
    displayedInputDb = ampDb(processor.getInputPeak());
    displayedOutputDb = ampDb(processor.getOutputPeak());
    displayedRideDb = processor.getRideDb();
    displayedPeakReductionDb = processor.getLimiterReductionDb();
    displayedRmsDb = processor.getDetectedRmsDb();
    displayedBlockRmsDb = ampDb(processor.getInputBlockRms());
    displayedRequestedDb = processor.getRequestedRideDb();
    displayedActive = processor.getRiderActive();
    const auto dsp = processor.getDSPSnapshot();
    processor.rememberDSPSnapshot(dsp);
    displayedAverageLoad = dsp.average;
    displayedPeakLoad = dsp.peak;
    displayedRollingPeak = dsp.rollingPeak;
    displayedSpikes = dsp.spikes;
    displayedCallbackUs = dsp.callbackUs;
    displayedLastSpikePercent = dsp.lastSpikePercent;
    displayedLastSpikeUs = dsp.lastSpikeUs;
    displayedLastSpikeBudgetUs = dsp.lastSpikeBudgetUs;
    displayedGuardToggles = dsp.guardToggles;
    displayedLastTogglePercent = dsp.lastTogglePercent;
    displayedLastToggleRiderPercent = dsp.lastToggleRiderPercent;
    comparisonInfo.setText("Output RMS " + juce::String(processor.getRecentOutputRmsDb(), 1)
       + " dBFS   |   Match " + juce::String(processor.getABMatchingDb(), 1) + " dB"
       + "   |   Floor " + juce::String(processor.getEffectiveFloorDb(), 1) + " dB",
       juce::dontSendNotification);
    gainHistory[(size_t)historyPosition] = displayedRideDb;
    rmsHistory[(size_t)historyPosition] = displayedRmsDb;
    historyPosition = (historyPosition + 1) % (int)gainHistory.size();
    repaint(juce::Rectangle<int>(18, 8, getWidth()-36, 266));
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
}

void SmoothRideAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> r,
                                               float levelDb, const juce::String& name) const {
    g.setColour(faint);
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText(name, r.removeFromTop(21).toNearestInt(), juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff344353));
    g.fillRoundedRectangle(r.reduced(0, 8), 5.0f);
    const float fraction = juce::jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);
    auto fill = r.reduced(0, 8);
    fill.setWidth(fill.getWidth() * fraction);
    g.setColour(levelDb > -3.0f ? juce::Colour(0xffffa65c) : accent);
    g.fillRoundedRectangle(fill, 5.0f);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(juce::String(levelDb, 1) + " dBFS", r.toNearestInt(), juce::Justification::centredRight);
}

void SmoothRideAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    constexpr int margin = 18;
    const int right = getWidth() - margin;
    const int innerWidth = getWidth() - 2 * margin;

    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("bold")));
    g.drawText("SMOOTHRIDE", margin, 8, 245, 35, juce::Justification::centredLeft);
    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("v0.6.4  |  GAIN RIDER", 878, 10, right-878, 30, juce::Justification::centredRight);
    g.setColour(faint);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("AVG " + juce::String(displayedAverageLoad, 2) + "%"
               + "  PEAK " + juce::String(displayedPeakLoad, 2) + "%"
               + "  60s " + juce::String(displayedRollingPeak, 2) + "%"
               + "  SPIKES " + juce::String(static_cast<juce::int64>(displayedSpikes))
               + "  LAST " + juce::String(displayedCallbackUs, 0) + " us",
               270, 12, 608, 24, juce::Justification::centredLeft);

    g.setColour(faint);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("LAST SPIKE " + juce::String(displayedLastSpikePercent, 2) + "%  ("
               + juce::String(displayedLastSpikeUs, 1) + " / "
               + juce::String(displayedLastSpikeBudgetUs, 1) + " us)",
               272, 32, 386, 14, juce::Justification::centredLeft);
    g.drawText("GUARD TOGGLES " + juce::String(static_cast<juce::int64>(displayedGuardToggles))
               + "  BLOCK " + juce::String(displayedLastTogglePercent, 1) + "%"
               + "  RIDER " + juce::String(displayedLastToggleRiderPercent, 1) + "%",
               660, 32, 396, 14, juce::Justification::centredRight);

    // Wide, single-row peak meters: all coordinates stay inside 1080 x 600.
    g.setColour(panel);
    g.fillRoundedRectangle(juce::Rectangle<float>((float)margin, 49.f, (float)innerWidth, 63.f), 8.f);
    drawMeter(g, {31.f, 55.f, 405.f, 49.f}, displayedInputDb, "INPUT PEAK");
    drawMeter(g, {451.f, 55.f, 405.f, 49.f}, displayedOutputDb, "OUTPUT PEAK");
    g.setColour(faint);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("LIMITER", 865, 56, 180, 20, juce::Justification::centred);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(17.0f).withStyle("bold")));
    g.drawText(juce::String(-displayedPeakReductionDb, 1) + " dB", 865, 77, 180, 25, juce::Justification::centred);

    const auto chart = juce::Rectangle<float>((float)margin, 120.f, (float)innerWidth, 72.f);
    g.setColour(panel);
    g.fillRoundedRectangle(chart, 8.f);
    g.setColour(faint);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("RIDE (+/-12 dB)  /  RMS (-100..0 dBFS)  |  NOISE REF "
               + (processor.apvts.getRawParameterValue("noiseGuard")->load() >= 0.5f
                   ? juce::String(processor.getNoiseReferenceDb(), 0) + " dB" : "OFF"),
               31, 123, 625, 17,
               juce::Justification::centredLeft);
    g.setColour(accent);
    g.drawText((displayedRideDb >= 0.f ? "+" : "") + juce::String(displayedRideDb, 1) + " dB",
               right-148, 123, 132, 17, juce::Justification::centredRight);
    const auto plot = juce::Rectangle<float>(34.f, 144.f, (float)getWidth()-68.f, 40.f);
    g.setColour(juce::Colour(0xff516272));
    g.drawHorizontalLine((int)plot.getCentreY(), plot.getX(), plot.getRight());
    juce::Path path;
    for (size_t k=0; k<gainHistory.size(); ++k) {
        const auto idx = (historyPosition + (int)k) % (int)gainHistory.size();
        const float x = plot.getX() + (float)k / (float)(gainHistory.size()-1) * plot.getWidth();
        const float y = plot.getCentreY() - juce::jlimit(-12.f, 12.f, gainHistory[(size_t)idx]) / 12.f * plot.getHeight()/2.f;
        if (k == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }
    g.setColour(accent);
    g.strokePath(path, juce::PathStrokeType(2.0f));
    // Blue trace: detector RMS, separately scaled from gain to -100..0 dBFS.
    juce::Path rmsPath;
    for (size_t k=0; k<rmsHistory.size(); ++k) {
        const auto idx = (historyPosition + (int)k) % (int)rmsHistory.size();
        const float x = plot.getX() + (float)k / (float)(rmsHistory.size()-1) * plot.getWidth();
        const float normalized = juce::jlimit(0.f, 1.f, (rmsHistory[(size_t)idx] + 100.f) / 100.f);
        const float y = plot.getBottom() - normalized * plot.getHeight();
        if (k == 0) rmsPath.startNewSubPath(x, y); else rmsPath.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xff68a7ff));
    g.strokePath(rmsPath, juce::PathStrokeType(1.5f));

    // Diagnostics are read-only. Never alter DSP state in the editor.
    g.setColour(panel);
    g.fillRoundedRectangle(juce::Rectangle<float>((float)margin, 200.f, (float)innerWidth, 65.f), 8.f);
    const int columnWidth = (innerWidth - 30) / 3;
    const auto drawDiagnostic = [&](int col, const juce::String& label,
                                     const juce::String& value, juce::Colour valueColour) {
        const int x = 32 + col * (columnWidth + 10);
        g.setColour(faint);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(label, x, 205, columnWidth, 17, juce::Justification::centredLeft);
        g.setColour(valueColour);
        g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("bold")));
        g.drawText(value, x, 227, columnWidth, 27, juce::Justification::centredLeft);
    };
    drawDiagnostic(0, "DETECTOR / BLOCK RMS", juce::String(displayedRmsDb, 1) + " / "
                   + juce::String(displayedBlockRmsDb, 1) + " dBFS", text);
    drawDiagnostic(1, "REQUESTED RIDE", (displayedRequestedDb >= 0.f ? "+" : "")
                   + juce::String(displayedRequestedDb, 1) + " dB", text);
    const auto* boostParam = processor.apvts.getRawParameterValue("range");
    const auto* cutParam = processor.apvts.getRawParameterValue("maxCut");
    const float boost = boostParam != nullptr ? boostParam->load() : 6.f;
    const float cut = cutParam != nullptr ? cutParam->load() : 3.f;
    const juce::String status = !displayedActive ? "BELOW FLOOR"
        : displayedRequestedDb >= boost-0.05f && boost>0.f ? "MAX BOOST"
        : displayedRequestedDb <= -cut+0.05f && cut>0.f ? "MAX CUT" : "ACTIVE";
    const bool adaptiveOn = processor.apvts.getRawParameterValue("adaptiveFloor")->load() >= 0.5f;
    const juce::String detail = !adaptiveOn ? "MANUAL" : processor.getSilenceBlocked()
        ? "SILENCE" : processor.getNoiseBlocked() ? "NOISE GUARD" : "ADAPTIVE";
    drawDiagnostic(2, "RIDER STATUS / " + detail,
        processor.getNoiseBlocked() ? "NOISE BLOCKED" : status,
        displayedActive ? accent : faint);

    // Compact cards leave two rows for user presets and A/B snapshots.
    constexpr int cols = 4, cardY = 272, cardGap = 6, cardH = 66;
    const int cardW = (innerWidth - (cols-1)*cardGap) / cols;
    for (int i=0; i<numControls; ++i) {
        const int x = margin + (i%cols)*(cardW+cardGap);
        const int y = cardY + (i/cols)*(cardH+cardGap);
        g.setColour(panel);
        g.fillRoundedRectangle((float)x, (float)y, (float)cardW, (float)cardH, 8.f);
    }
}

void SmoothRideAudioProcessorEditor::resized() {
    constexpr int cols = 4, cardY = 272, cardGap = 6, cardH = 66;
    const int cardW = (getWidth()-36-(cols-1)*cardGap)/cols;
    for (int i=0; i<numControls; ++i) {
        const int x = 18+(i%cols)*(cardW+cardGap);
        const int y = cardY+(i/cols)*(cardH+cardGap);
        labels[(size_t)i]->setBounds(x+6, y+1, cardW-12, 14);
        sliders[(size_t)i]->setBounds(x+8, y+14, cardW-16, cardH-15);
    }
    presetLabel.setBounds(22, 494, 155, 23);
    presets.setBounds(178, 493, 300, 25);
    presetName.setBounds(489, 493, 244, 25);
    savePreset.setBounds(739, 493, 71, 25);
    notice.setBounds(815, 493, 244, 25);
    userPresetList.setBounds(20, 525, 270, 26);
    loadPreset.setBounds(298, 525, 77, 26);
    deletePreset.setBounds(380, 525, 77, 26);
    captureA.setBounds(485, 525, 85, 26);
    captureB.setBounds(575, 525, 85, 26);
    recallA.setBounds(672, 525, 85, 26);
    recallB.setBounds(762, 525, 85, 26);
    resetDSP.setBounds(860, 525, 90, 26);
    spikeThreshold.setBounds(956, 525, 100, 26);
    noiseGuard.setBounds(660, 120, 195, 22);
    matchAB.setBounds(20, 559, 128, 25);
    calibrateA.setBounds(160, 557, 80, 27);
    calibrateB.setBounds(248, 557, 80, 27);
    comparisonInfo.setBounds(347, 556, 420, 28);
    importPreset.setBounds(784, 557, 125, 27);
    exportPreset.setBounds(918, 557, 125, 27);
}
