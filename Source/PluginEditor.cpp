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
         "GAIN UP", "GAIN DOWN", "MIX", "OUTPUT", "CEILING", "FLOOR HYST", "SOFT FLOOR" }};
    for (int i = 0; i < numControls; ++i) {
        sliders[i] = std::make_unique<juce::Slider>();
        auto& slider = *sliders[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 115, 25);
        slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff354555));
        slider.setColour(juce::Slider::thumbColourId, text);
        slider.setColour(juce::Slider::textBoxTextColourId, text);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, panel);
        if (ids[i] == "speed" || ids[i] == "downSpeed") slider.setTextValueSuffix(" ms");
        else if (ids[i] == "lowProtect") slider.setTextValueSuffix(" (0=OFF, 1=ON)");
        else if (ids[i] == "mix") slider.setTextValueSuffix(" %");
        else slider.setTextValueSuffix(" dB");
        addAndMakeVisible(slider);
        labels[i] = std::make_unique<juce::Label>();
        labels[i]->setText(names[i], juce::dontSendNotification);
        labels[i]->setJustificationType(juce::Justification::centred);
        labels[i]->setColour(juce::Label::textColourId, faint);
        labels[i]->setFont(juce::Font(juce::FontOptions(13.0f).withStyle("bold")));
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
    gainHistory.fill(0.0f);
    setSize(900, 1090);
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
    gainHistory[(size_t)historyPosition] = displayedRideDb;
    historyPosition = (historyPosition + 1) % (int)gainHistory.size();
    repaint(juce::Rectangle<int>(18, 60, getWidth()-36, 300));
}

void SmoothRideAudioProcessorEditor::applyPreset(int presetId) {
    // Optional parameter changes on the UI thread, never the audio thread.
    // The baseline exactly matches the v0.2 defaults; all presets retain safe limited riding.
    const std::array<float, 11> baseline {{-12.f, -60.f, 6.f, 3.f, 3000.f, 650.f, 65.f, 6.f, -1.f, 0.f, 0.f}};
    const std::array<float, 11> music    {{-12.f, -60.f, 4.f, 2.f, 4000.f, 900.f, 45.f, 6.f, -1.f, 0.f, 0.f}};
    const std::array<float, 11> streams  {{-12.f, -60.f, 6.f, 3.f, 3000.f, 650.f, 65.f, 6.f, -1.f, 0.f, 0.f}};
    const std::array<float, 11> movies   {{-14.f, -60.f, 3.f, 2.f, 5000.f, 1100.f, 40.f, 4.f, -1.f, 0.f, 0.f}};
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
    auto top = juce::Rectangle<float>(20, 13, (float)getWidth()-40, 44);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(28.0f).withStyle("bold")));
    g.drawText("SMOOTHRIDE", top.removeFromLeft(240).toNearestInt(), juce::Justification::centredLeft);
    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("v0.3.2  |  STEREO-LINKED GAIN RIDER", top.toNearestInt(), juce::Justification::centredRight);

    const auto meterArea = juce::Rectangle<float>(20, 67, (float)getWidth()-40, 90);
    g.setColour(panel);
    g.fillRoundedRectangle(meterArea, 10.0f);
    drawMeter(g, {35.f, 80.f, 325.f, 55.f}, displayedInputDb, "INPUT PEAK");
    drawMeter(g, {380.f, 80.f, 325.f, 55.f}, displayedOutputDb, "OUTPUT PEAK");
    g.setColour(faint);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("PEAK REDUCTION", 730, 83, 145, 20, juce::Justification::centred);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("bold")));
    g.drawText(juce::String(-displayedPeakReductionDb, 1) + " dB", 730, 105, 145, 30, juce::Justification::centred);

    auto chart = juce::Rectangle<float>(20, 169, (float)getWidth()-40, 100);
    g.setColour(panel);
    g.fillRoundedRectangle(chart, 10.0f);
    g.setColour(faint);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("GAIN RIDING HISTORY  |  LAST 24 s", 35, 172, 330, 20, juce::Justification::centredLeft);
    g.setColour(accent);
    g.drawText((displayedRideDb >= 0.f ? "+" : "") + juce::String(displayedRideDb, 1) + " dB",
               getWidth()-167, 172, 135, 20, juce::Justification::centredRight);
    const auto plot = chart.reduced(19.f, 26.f).withTrimmedTop(4.f);
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

    // Pure telemetry strip: no audio parameters or processing modified.
    const float diagnosticsY = 282.0f;
    g.setColour(panel);
    g.fillRoundedRectangle(20.0f, diagnosticsY, (float)getWidth()-40.0f, 76.0f, 10.0f);
    const float columnWidth = ((float)getWidth()-70.0f)/3.0f;
    const auto drawDiagnostic = [&](int col, const juce::String& label,
                                     const juce::String& value, juce::Colour valueColour) {
        const int x = 35 + (int)((float)col*(columnWidth+5.0f));
        g.setColour(faint);
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText(label, x, 292, (int)columnWidth, 20, juce::Justification::centredLeft);
        g.setColour(valueColour);
        g.setFont(juce::Font(juce::FontOptions(19.0f).withStyle("bold")));
        g.drawText(value, x, 318, (int)columnWidth, 28, juce::Justification::centredLeft);
    };
    drawDiagnostic(0, "DETECTOR / BLOCK RMS", juce::String(displayedRmsDb, 1) + " / " + juce::String(displayedBlockRmsDb, 1) + " dBFS", text);
    drawDiagnostic(1, "REQUESTED RIDE", (displayedRequestedDb >= 0.0f ? "+" : "")
                   + juce::String(displayedRequestedDb, 1) + " dB", text);
    const auto* floorParam = processor.apvts.getRawParameterValue("floor");
    const bool belowFloor = !displayedActive;
    const auto* boostParam = processor.apvts.getRawParameterValue("range");
    const auto* cutParam = processor.apvts.getRawParameterValue("maxCut");
    const float boost = boostParam != nullptr ? boostParam->load() : 6.0f;
    const float cut = cutParam != nullptr ? cutParam->load() : 3.0f;
    const juce::String riderStatus = belowFloor ? "BELOW FLOOR"
       : displayedRequestedDb >= boost - 0.05f && boost > 0.0f ? "MAX BOOST"
       : displayedRequestedDb <= -cut + 0.05f && cut > 0.0f ? "MAX CUT"
       : "ACTIVE";
    juce::ignoreUnused(floorParam);
    drawDiagnostic(2, "RIDER STATUS", riderStatus,
                   belowFloor ? faint : accent);

    const int cardX = 20, cardY = 375, cardGap = 10, cardW = (getWidth()-40-2*cardGap)/3, cardH = 140;
    for (int i=0; i<numControls; ++i) {
        const int x = cardX + (i%3)*(cardW+cardGap);
        const int y = cardY + (i/3)*(cardH+cardGap);
        g.setColour(panel);
        g.fillRoundedRectangle((float)x, (float)y, (float)cardW, (float)cardH, 9.f);
    }
    g.setColour(faint);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("Presets change the eleven controls above. Current values are saved by the host.",
               20, 1053, getWidth()-40, 22, juce::Justification::centred);
}

void SmoothRideAudioProcessorEditor::resized() {
    const int cardX = 20, cardY = 375, cardGap = 10, cardW = (getWidth()-40-2*cardGap)/3, cardH = 140;
    for (int i=0; i<numControls; ++i) {
        const int x = cardX + (i%3)*(cardW+cardGap);
        const int y = cardY + (i/3)*(cardH+cardGap);
        labels[(size_t)i]->setBounds(x+10, y+5, cardW-20, 22);
        sliders[(size_t)i]->setBounds(x+20, y+26, cardW-40, cardH-30);
    }
    presetLabel.setBounds(28, 1017, 205, 23);
    presets.setBounds(235, 1009, getWidth()-263, 34);
}
