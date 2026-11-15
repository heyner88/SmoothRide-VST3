# SmoothRide v0.3.3 — compact host-safe editor

## Changes
- Editor changed from 900 x 1090 to **1080 x 600** logical pixels.
- 4-column, 3-row parameter layout. Readouts, history and presets kept visible.
- Calls `setResizable(false, false)` to prevent plugin-initiated resizes in hosts.
- No DSP, parameter identifier, serialization or default changes.
- Editor version label updated to 0.3.3.

## Caveat
This is a mitigation for host/editor geometry problems, not proof that a hidden modal window cannot happen. The title-bar and host-owned window management are controlled by EqualizerAPO-XT. If it happens again, try Alt+F4 or Alt+Space then M before terminating the host.

## Update
Replace the repository files with the ZIP contents, let GitHub Actions build on Windows, close the host/plugin window, back up the previous VST3 bundle, and replace it. Restore the existing listening settings.
