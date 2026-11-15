# SmoothRide v0.6.4 — Noise Guard toggle timing

This diagnostic release preserves the v0.6.3 gain rider and all audio settings.

- **GUARD TOGGLES** counts observed Noise Guard state changes on the audio thread since Reset DSP.
- **BLOCK %** is the entire audio callback's time-budget use on the most recent toggle.
- **RIDER %** is the measured GainRider processing portion of that same callback.
- **LAST SPIKE** / **PEAK** / **PEAK 60s** and the spike counter are unchanged.
- Reset DSP clears all toggle and spike diagnostics.

Measurements use wall-clock time, so Windows scheduling can affect either number. A large BLOCK % and small RIDER % would suggest the spike is outside the gain-rider's processing window; it would not prove why.

## Suggested test

1. Play a livestream. Set the spike threshold to 10%, reset DSP.
2. Toggle Noise Guard ON once; record GUARD TOGGLES, BLOCK %, RIDER %, and SPIKES.
3. Toggle OFF; record the same values.
4. Confirm audio remains smooth.

**Windows JUCE compile and EqualizerAPO-XT behavior must be validated on your system.**
