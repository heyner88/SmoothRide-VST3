# SmoothRide v0.6.6 — PRE-stage timing diagnostics

This version builds on v0.6.5 and does not change the gain rider, Noise Guard, preset serialization, or A/B audio behavior.

## Diagnostics

- **P**: time spent before input analysis, including parameter reads and callback initialization.
- **I**: input peak and RMS analysis.
- **AB**: A/B transition preparation, including the old gain rider when crossfading.
- **AB***: a crossfade actually ran during the last Noise Guard toggle.
- **R**: current gain-rider processing.
- **O**: output/postprocessing and metering.
- **NORMAL PRE**: exponentially smoothed PRE timing for callbacks without a Noise Guard toggle; useful as a comparison baseline, not a CPU average.

All phase percentages are relative to the audio-block time budget. Displayed toggle phase values are captured on the first audio callback that observes the new Noise Guard setting. Measurements use elapsed wall-clock time and may include OS scheduling interruptions. The timing probes themselves add overhead.

## Test

1. With a livestream playing, set the spike threshold to 10% and Reset DSP.
2. Leave Noise Guard OFF for 30 seconds and note NORMAL PRE.
3. Enable Noise Guard once; note BLOCK, P, I, AB, R, O, and AB* indicator.
4. Disable Noise Guard once and compare; repeat after another reset if helpful.
5. Verify A/B switching and presets still work as in v0.6.5.

The GitHub Actions workflow compiles the VST3, runs regression tests, validates with pluginval and packages the binary. The Windows build and EqualizerAPO-XT listening test must still be performed.
