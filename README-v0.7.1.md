# SmoothRide v0.7.1 — Lightweight vs detailed DSP monitoring

Maintenance update based on v0.7.0. Core DSP, parameter cache, Adaptive Floor, Noise Guard, and all 13 sound parameters are unchanged.

## New: DETAIL DSP

- OFF (default): only total callback timing and AVG/PEAK/60s/SPIKES metrics; intermediate P/I/AB/R/O timestamps are skipped.
- ON: full per-stage breakdown and event capture as in v0.7.0.
- Changing DETAIL DSP requests a telemetry reset on the next audio block, so measurements from different modes cannot mix. The toggle is session-only and does not affect saved sound presets.
- A raw Noise Guard transition count is shown even in lightweight mode. A raw transition is not the same as a qualifying spike.
- Timing is still wall-clock-based, so Windows scheduling interruptions remain possible. No timing mode disables all telemetry.

## How to compare

1. With Noise Guard OFF, run 30 seconds of audio with DETAIL DSP OFF, reset and record normal AVG/PEAK/SPIKES.
2. Enable DETAIL DSP, reset and repeat using the same content.
3. In DETAIL mode, reset and toggle Noise Guard ON/OFF five complete cycles (ten changes), and compare T transitions to COUNTS T spikes.
4. Return to OFF for everyday use; the plugin retains gain riding and basic performance metrics.

## Validation

Standalone C++ telemetry/DSP tests can run locally. Windows JUCE compilation, pluginval and XT listening must be completed through GitHub Actions / XT.
