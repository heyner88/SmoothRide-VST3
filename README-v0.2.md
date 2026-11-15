# SmoothRide v0.2 — asymmetric gain riding

This is the original SmoothRide v0.1 project updated for more natural quiet-passage audibility. It is **not** W.A. Production's Outlaw code and its settings are not exact equivalents.

## Changes
- `range` (old parameter ID retained) is now **Max Boost (dB)**. **Important:** v0.1's symmetric `range` value becomes an *upward-only* limit; check presets after upgrading.
- New **Max Cut (dB)**, default 3 dB.
- `speed` (old parameter ID retained) is now **Gain Up Speed (ms)**, default 3000 ms.
- New **Gain Down Speed (ms)**, default 650 ms.
- Default Target RMS raised from −15 to −12 dB to reflect real listening tests.
- Existing Mix, Floor, Output, Ceiling parameter identifiers retained.
- Stereo-linked RMS detector and instant sample-peak safety ceiling retained (not a true-peak / lookahead limiter).
- More DSP tests for asymmetrical caps.

## Suggested initial settings
Target −12 dB RMS; Floor −60 dB; Max Boost +6 dB; Max Cut 3 dB; Gain Up 3000 ms; Gain Down 650 ms; Mix 65%; Output +6 dB; Ceiling −1 dBFS.

**Safety:** Start at reduced system volume and verify the signal isn't severely limited. Upward gain can raise noise as well as quiet program content. These are exploratory defaults, not measured calibration.

## Update GitHub repository
Upload the **contents** of this folder to the existing repository root, replacing same-path files. Ensure `.github/workflows/build-windows.yml` is present; use Actions → Build SmoothRide VST3 (Windows) → Run workflow. A new run produces a fresh artifact. Don't upload the `JUCE` folder into GitHub.

This version has been tested at the standalone C++ DSP level only; the JUCE VST3 build and XT behavior need Windows CI and listening verification.
