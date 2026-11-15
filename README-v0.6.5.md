# SmoothRide v0.6.5 — Callback Phase Diagnostics

Maintenance release based on v0.6.4; audio processing, presets, Adaptive Floor, and Noise Guard are unchanged.

## New diagnostics

When Noise Guard toggles, the top-row measurements show:

- **BLOCK %**: elapsed time from audio callback entry to immediately before performance telemetry, divided by the audio-block time budget.
- **PRE %**: callback entry to the beginning of `rider.process`, including input analysis and optional A/B transition setup.
- **RIDER %**: elapsed time inside the current configuration's `rider.process` call.
- **POST %**: after the current rider call through output-processing and metering, before telemetry.
- **MON us**: time spent publishing and updating telemetry in the **previous callback**; deliberately excluded from BLOCK. The value is one-block delayed, not necessarily the toggle block's monitoring time.

These are elapsed wall-clock measurements, not CPU utilization; an OS scheduling interruption can inflate any segment. PRE includes the previous rider when a preset A/B crossfade is in progress. Percentages may differ slightly due to floating-point rounding.

## Suggested test

1. Play a familiar livestream, set spike threshold to 10%, then click Reset DSP.
2. Enable Noise Guard once; capture BLOCK, PRE, RIDER, POST, MON, and SPIKES.
3. Disable Noise Guard and capture the same values.
4. Repeat with no audio glitches. Avoid inferring causality from a single pair of spikes.

The standalone DSP/performance tests run in CI. The Windows JUCE build and listening test must still be completed.
