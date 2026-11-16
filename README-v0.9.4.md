# SmoothRide v0.9.4 — metering and advanced UI polish

Full JUCE 8.0.12 Windows x64 VST3 source package for the included GitHub Actions workflow.

## Improvements over v0.9.3
- Independent left/right **peak-hold markers** in both input and output meter panels. Markers hold ~1.2 seconds and decay at 20 dB/s. This is UI-side only and uses the existing channel peak readings; it does not modify the DSP or introduce audio-thread allocations.
- Advanced DSP readouts have a clearer lead line and a subtle divider.
- Active preset status has a colored indicator; preset status now repaints as host automation modifies the active preset.
- Fixed **1080 x 600** layout, parameter IDs and preset format unchanged.

## Validation
The standalone DSP/performance/parameter-cache/preset-policy tests are supplied in `tests/`. A Windows JUCE compilation and EqualizerAPO-XT visual/audio test are still required.
