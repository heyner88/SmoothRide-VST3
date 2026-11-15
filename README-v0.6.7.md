# SmoothRide v0.6.7 — Reset-isolated parameter telemetry

This diagnostic release preserves the v0.6.6 audio DSP, presets, Noise Guard, and 1080×600 GUI.

## New diagnostics

- **BLOCK** directly shows the most recent Noise Guard toggle callback time budget percentage; this is **not** historical PEAK.
- **BASE P** shows the exponentially smoothed parameter/preparation time during normal, non-toggle callbacks with three decimal places.
- **P AVG/MAX** shows mean and maximum P percentages over Noise Guard toggle callbacks since Reset DSP (not all audio blocks).
- **RESET us** times the reset bookkeeping separately and excludes it from normal callback BLOCK and P measurements.
- The historical PEAK, rolling 60-second PEAK and SPIKES statistics remain unchanged in meaning.

## Test protocol

1. Start a livestream; keep Adaptive Floor ON and Noise Guard OFF. Set spike threshold 10%.
2. Press Reset DSP, then allow 30 seconds of ordinary audio to establish BASE P.
3. Enable Noise Guard; record BLOCK, P, I, AB, R, O, BASE P, P AVG/MAX, RESET us and SPIKES.
4. Disable Noise Guard; record again, without resetting.
5. Toggle three or more additional times, then compare P AVG/MAX to BASE P and check audio smoothness.

**Important:** Wall-clock timers include OS scheduling interruptions, so large P measurements do not by themselves prove that parameter reads consume equivalent CPU time. Timers add some overhead. Windows JUCE build and XT testing are pending.
