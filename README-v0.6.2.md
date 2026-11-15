# SmoothRide v0.6.2 — Performance Diagnostics

This release is based on v0.6.1 and keeps the DSP algorithm and existing preset format unchanged.

**New UI**: Reset DSP button, rolling 60-second DSP peak, configurable spike threshold (5/10/20/40%; defaults 10%) and total spike counter.

**Important**: PEAK retains the maximum value recorded since the last reset; 60s is the last 60 seconds of monotonic wall time, refreshed when processing resumes; AVG is exponentially smoothed. The spike counter counts **audio callbacks** reaching the threshold, not unique user events. These are percentages of each block's real-time execution budget, not Windows CPU utilization.

Clicking Reset DSP does not touch audio settings or presets. The audio thread clears the statistics at the beginning of its next callback.

**Validation**: Compile via GitHub Actions, check panel layout and counters, play a livestream, toggle Noise Guard, then reset metrics and compare. Keep v0.6.1 as fallback until tested.
