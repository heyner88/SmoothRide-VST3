# SmoothRide v0.7.2 — Listening view and preset clarity

## Changes

- New **LISTEN VIEW** toggle (ON by default), which hides detailed developer telemetry and DSP diagnostic controls.
- Main signal meters, gain riding/RMS graph, rider status, all 12 parameter controls, Noise Guard, and preset management remain accessible.
- Listening view shows selected saved preset name and clarifies that clicking **Load** is required to apply it.
- Uncheck LISTEN VIEW to restore full advanced diagnostics and the existing DETAIL DSP, Reset DSP, and spike threshold controls.
- No audio engine changes. Editor remains fixed at 1080 x 600.
- LISTEN VIEW is local to the editor, defaults to ON when opening the panel, and is not serialized in host state.

## Suggested validation

Build through GitHub Actions; confirm default listening view, switch to advanced and back, load/save/export/import presets, A/B switch, restart XT, and verify smooth playback.
