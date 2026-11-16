# SmoothRide v0.6.9 — Callback spike isolation

**Experimental telemetry release.** Built on v0.6.8. Audio DSP, parameter-pointer caching,
Noise Guard and A/B behavior are unchanged.

## New diagnostics (bottom of the Rider Status panel)

- **PEAK**: stage breakdown captured from the callback that set historical PEAK.
- **SPIKE**: stage breakdown captured from the most recent callback that exceeded the spike threshold.
- Each record shows **BLOCK P I AB R O** in percentages of its own audio-block deadline.
- **T** identifies a Noise Guard transition on that specific callback; **N** means normal/non-toggle callback.
- An `*` following AB indicates the previous A/B engine ran during that callback.
- **COUNTS T / N**: threshold-crossing callbacks during Noise Guard transitions versus other callbacks. Counts reset with RESET DSP.
- Both snapshots remain until replaced by a qualifying callback or RESET DSP.
- Timer readings are elapsed wall time, which may include pre-emption by the operating system. Attribution to a code stage does not prove CPU cost originated there.

## Test instructions

1. Back up working v0.6.8 and build v0.6.9 with GitHub Actions (same procedure).
2. With Adaptive Floor ON, Noise Guard OFF and threshold 10%, reset DSP and play for 30 seconds.
3. Record PEAK and SPIKE breakdowns and COUNTS T / N without toggling anything.
4. Reset DSP, then toggle Noise Guard ON and OFF five times. Record both breakdowns and COUNTS T / N.
5. Verify audio remains smooth and presets/A/B controls still work.
6. If peak is high, share P/I/AB/R/O for the **PEAK** row rather than the latest toggle row.

## Limitations

The plugin has not been built with JUCE on this Linux runtime. Standalone C++ telemetry and DSP tests can be run locally; Windows compilation, pluginval and XT listening tests remain necessary.
