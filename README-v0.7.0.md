# SmoothRide v0.7.0 — Correlated spike events

This is a **diagnostic-only** update based on v0.6.9. DSP processing and parameter caching are unchanged.

## Improvements

- Historical `PEAK` now comes from the same captured peak callback event as its P/I/AB/R/O breakdown.
- `PEAK #` and `SPIKE #` show callback sequence identifiers since the most recent Reset DSP or reinitialization.
- The event's `T` label means a Noise Guard state change was detected in that exact callback; `N` means no state change was detected then. These labels indicate correlation, not causation.
- Added regression checks for peak/event agreement, event IDs, reset semantics, and snapshot publication.

## Controlled benchmark

1. Run a livestream with Noise Guard OFF, press Reset DSP, and avoid other controls for 30 seconds.
2. Record the headline PEAK percentage and the `PEAK #` event percentage/ID. They must be equal to within display rounding.
3. Reset again, toggle Noise Guard ON/OFF five complete cycles, and record PEAK and SPIKE event IDs, T/N labels, stage timings, and counts.
4. Confirm sound is unchanged. No noise-guard DSP optimization is included yet.

The callback wall-clock timer includes potential OS scheduling interruptions. PEAK and SPIKE event IDs are scoped to the current monitoring-reset interval.
