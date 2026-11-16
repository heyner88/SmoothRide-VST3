# SmoothRide v0.9.0 — Mastering-inspired JUCE interface (preview build)

Based on the validated v0.8.0 project. The audio processor and its DSP/parameter/preset persistence logic remain byte-for-byte unchanged.

## Changes
- New dark, teal-accented look and feel, including custom circular slider graphics.
- Larger vertical input/output peak meters. **These are mono peak readouts, not independent L/R channels** (per-channel peaks are not exposed by the current processor).
- Expanded gain-riding and detector RMS history chart in Listening view.
- Four dedicated parameter groups: LEVELING (Target RMS, Max Boost, Max Cut), RESPONSE (Gain Up, Gain Down, Mix), DETECTION (Floor, Hysteresis, Soft Floor, Adaptive Floor, Noise Guard), and OUTPUT (Output Gain, Ceiling).
- Advanced view uses the same main panel for existing DSP peak/spike/callback measurements.
- Factory presets, user presets, Save, Load, Update, Revert, Delete, Import, Export, A/B capture and recall, A/B calibration, and A/B matching remain available.
- Fixed 1080x600 window to retain EqualizerAPO-XT compatibility.

## Validation status
C++ DSP regression tests can be run locally without JUCE. **JUCE GUI/VST3 compilation has not been verified locally**: build and test on Windows via GitHub Actions before adopting this release.

## Windows test checklist
1. Compile VST3 successfully and pass pluginval.
2. Confirm window opens/closes/reopens at 1080x600 without clipping, especially controls along the bottom.
3. Test all 12 parameter knobs, Noise Guard and view toggle.
4. Test preset Save/Load/Update/Revert, factory selection, import/export, A/B Store/Recall, calibration and matching.
5. Verify meters, graph, diagnostics and status values during silence and active gain riding.
6. Compare sound, latency, and DSP AVG against v0.8.0.

The three design mockups are visual direction; this implementation adapts them to a smaller host-compatible editor and does not claim pixel-perfect visual parity.
