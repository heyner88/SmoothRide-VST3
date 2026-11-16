# SmoothRide v0.9.2 — stereo meters and interface polish

Based on tested v0.9.1, preserving its gain-rider settings, presets and real-time processing.

## Changes
- True independent input/output L/R peak meters (audio-buffer magnitudes, not duplicated mono values). For mono audio the unused R meter reads silence.
- Improved meter geometry, clearer channel identification, knob contrast and labels.
- Cleaner Listening View footer showing preset selection, name, Save, Load, Update, Revert, Recall A/B, and matching toggle.
- Advanced View still contains all less-frequent preset, import/export, A/B storage and calibration actions plus detailed diagnostics.
- Fixed 1080 x 600 plugin window; no resizable host window.

## Important
Stereo readouts are read-only peak telemetry. The processed samples, gain riding, limiter, crossfade and preset behavior are unchanged. The two additional atomic stores per channel represent a minimal telemetry change in the processor and require live performance confirmation.

## Testing
1. Build the VST3 with Windows GitHub Actions, then install with XT closed.
2. Confirm L/R meters respond independently to stereo content; test mono content for no false R display.
3. Check Listening/Advanced transitions and ensure each preset action remains accessible.
4. Test preset load/save/update/revert and A/B recall; listen for unchanged smoothness.
5. Compare DSP AVG before and after during the same program material.
