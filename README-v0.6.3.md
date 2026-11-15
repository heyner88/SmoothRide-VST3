# SmoothRide v0.6.3 — Telemetry consistency update

- Maintains the v0.6.2 audio processing, Noise Guard, presets, and A/B behavior.
- Publishes DSP statistics together in one coherent, atomic snapshot for the GUI.
- Shows LAST SPIKE as percent of block deadline, actual callback microseconds and available block microseconds.
- Reset DSP resets historical peak, rolling peak, spike count, and last-spike details on the next audio callback.
- Adds a regression test asserting a 10% spike count implies historical PEAK >= 10% and LAST SPIKE >= 10%.

## User verification

Set threshold 10%; press Reset DSP; toggle Noise Guard once. If SPIKES becomes 1, historical PEAK and LAST SPIKE should both show at least 10%. Report those values alongside AVG and PEAK 60s. If historical peak stays below 10%, please provide a screenshot.

Test release: requires Windows JUCE build and EqualizerAPO-XT validation.
