# SmoothRide v0.9.2 — JUCE 8 compilation hotfix

Fixes the Windows/MSVC compile error in `Source/PluginEditor.cpp`: `juce::Graphics` has no `drawArc()` member. The decorative knob highlight now uses `juce::Path::addCentredArc()` plus `juce::Graphics::strokePath()`.

No audio processing, gain-riding DSP, parameter IDs, preset handling, or window size changed. Continue to use the existing v0.9.2 GitHub Actions workflow.
