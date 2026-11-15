# SmoothRide v0.6.8 — Parameter pointer caching

Targeted real-time parameter-path optimization based on v0.6.7.

- Bind all 13 APVTS atomic parameter pointers during processor construction.
- Replace 13 per-block string-ID lookups with direct atomic loads.
- Preserve live parameter automation, preset recall, A/B behavior, noise guard and audio DSP.
- Preserve all prior monitoring including BASE P, P AVG/P MAX, BLOCK and spike counting.
- Add portable C++ regression tests proving parameter changes are observed without rebinding.
- Existing DSP and telemetry tests remain in GitHub Actions.

## Test in EqualizerAPO-XT
1. Save a backup of your working v0.6.7 bundle and presets.
2. Build v0.6.8 via GitHub Actions, then install the versioned artifact.
3. With Adaptive Floor ON and Noise Guard OFF, press Reset DSP and let audio run 30 seconds; note BASE P.
4. Toggle Noise Guard ON and OFF five times with 2–3 seconds between changes. Record P AVG, P MAX, BLOCK, spikes and PEAK.
5. Verify that other controls, preset loading and A/B recall still take effect immediately and that audio stays smooth.

This is a **test release**. Lower toggle spikes are not guaranteed; large outliers could come from Windows scheduling. Windows JUCE compilation and host listening tests are not performed by this package creation step.
