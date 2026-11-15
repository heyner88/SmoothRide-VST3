# SmoothRide v0.4.2 — A/B comparisons

Built on v0.4.1. The GainRider.h file and its 11 parameter IDs are unchanged.

## New features

- **Store A / Store B**, then **Recall A / Recall B**: each recall triggers an 80 ms equal-power-independent linear crossfade between two copies of the processing state to avoid abrupt parameter changes. A second audio engine runs temporarily and may increase DSP load.
- **MATCH A/B** is disabled by default. It is a manual *attenuation-only* RMS comparison, not automatic LUFS matching. It never boosts beyond the safety ceiling.
- **Cal A / Cal B** captures a smoothed output RMS measurement into the corresponding stored A/B snapshot. Play at least 5 seconds of representative content with one slot active before each calibration. For the fairest comparison use the same music excerpt and same source player volume. Calibrate each slot with MATCH A/B disabled.
- On Recall A/B when matching is on and both slots are calibrated, the louder reference is attenuated to approximate the quieter one. The correction is capped at 6 dB attenuation. Neither source's original settings are modified.
- Matching data persists in the host state. Overwriting an A or B snapshot invalidates that snapshot's calibration.

## Important limitations

This is an *RMS-based, program-dependent* estimate; not LUFS, and not a replacement for independent loudness measurement. A single normalizer cannot know the loudness of both configurations on exactly the same live audio without running both permanently. For rigorous matching, compare with externally measured LUFS. A/B matching only runs after the user calibrates both slots.

The crossfade uses pre-allocated buffers for normal audio blocks up to 16384 frames. Larger blocks skip dual-path crossfade. CPU use increases during the 80 ms transition. If settings change via host automation rather than Recall A/B, the old GUI snapshots are not used for crossfading.

## Build

Use the existing GitHub Actions Windows workflow. Download its VST3 artifact, close XT, back up the previous bundle, and replace it. Confirm no regressions before turning on Match A/B.

## Suggested verification

1. Ensure your existing settings and named presets survive the upgrade.
2. Store two noticeably different configurations, test Recall A/B with Match disabled: check for clicks, dropouts, or level spikes.
3. With Match OFF, use the same representative audio passage for at least 5 seconds per slot, then press Cal A / Cal B respectively.
4. Turn Match ON, recall each slot, and compare perceived level. Keep speaker volume low at first.
5. Close and restart XT to verify saved slots and calibration persist.
