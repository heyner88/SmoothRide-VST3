# SmoothRide v0.3.2 — diagnostics and opt-in low-level controls

Build with the existing `.github/workflows/build-windows.yml` (JUCE cloned by CI).

## Changes
- Displays two RMS values: the **250 ms smoothed detector RMS** and an **unsmoothed input-buffer RMS**. Input Peak and Output Peak remain unchanged.
- Optional **FLOOR HYST** in dB. Default **0 dB**, exactly the v0.3.1 floor activation rule. Nonzero creates a hysteresis band centered on Floor; reduces toggling near the threshold.
- Optional **SOFT FLOOR** 0 = off, 1 = on. Default **off**. When enabled, positive gain correction fades from 0 at Floor to full over the following 12 dB. This limits boosting of very quiet material, but does not remove the fixed Output makeup gain.
- Existing parameter IDs, default values and algorithm remain unchanged when the two new options are off. Saved states from v0.3.1 should load into the same original controls; confirm in XT.
- Saved presets have the two new controls set to off.

## Practical test
1. Back up your known-good v0.3.1 VST3 and settings.
2. At the same Windows and browser volume, check Detector / Block RMS, Input Peak, Requested Ride and status.
3. Test FLOOR HYST 3 dB with SOFT FLOOR 0; listen for changes near the floor.
4. Test SOFT FLOOR 1 only if background noise is boosted at low listening levels.
5. Restore FLOOR HYST 0 and SOFT FLOOR 0 for the exact earlier gain algorithm.

The safety ceiling is a sample-peak ceiling, not a true-peak limiter. CI compilation and real XT testing are required; standalone DSP tests do not replace host validation.
