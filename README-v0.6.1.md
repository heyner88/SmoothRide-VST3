# SmoothRide v0.6.1 — Experimental Noise Guard and Diagnostics

## What's new
- Optional **NOISE GUARD (EXP)** toggle in the graph header; defaults OFF.
- Noise reference estimate and rider status diagnostics (MANUAL, ADAPTIVE, SILENCE, NOISE GUARD).
- Guard operates ONLY when Adaptive Floor is ON. If enabled, it can block boost on relatively steady low-level material below -55 dBFS that approaches a slowly tracked RMS reference (within 1.5 dB). This is **not** speech recognition or an accurate noise-only classifier; quiet music or steady dialogue may also be blocked.
- The v0.6.0 gain-riding behavior is preserved when Noise Guard is OFF.
- Presets export as format v3 (13 parameters); imports accept legacy formats v1/v2, setting Noise Guard OFF. Existing host state defaults missing Noise Guard to OFF.

## Testing
1. Leave Noise Guard OFF. Compare v0.6.0 Adaptive Floor behavior to v0.6.1; it should sound identical.
2. Check RMS graph and the new MANUAL/ADAPTIVE/SILENCE status. No DSP logic is changed in manual mode.
3. For experimentation, enable Noise Guard with Adaptive Floor ON. Listen to quiet noise-only passages and quiet speech/music. Disable it if quiet meaningful content is suppressed.
4. Confirm saved presets, import/export, A/B transitions, and GUI open/close work.
5. Run CTest and pluginval on Windows through GitHub Actions, then evaluate in XT.
