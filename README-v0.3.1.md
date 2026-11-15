# SmoothRide v0.3.1 — Diagnostics & interface polish

This is an incremental update to the user-tested v0.3 project. The audio gain-riding **calculations are unchanged** from v0.3. Only readonly telemetry snapshots were added in `GainRider.h` and forwarded through atomic values to the GUI.

## New on-screen diagnostics

- **Detected RMS:** stereo-linked detector level in dBFS; this is not peak level or LUFS.
- **Requested Ride:** desired gain change before time smoothing. This differs from the actual applied ride shown in the graph.
- **Rider Status:** `BELOW FLOOR`, `ACTIVE`, `MAX BOOST`, or `MAX CUT`, based on the detector and target gain. `BELOW FLOOR` means boost riding is inactive, not that audio is muted.
- Removed the Unicode bullet glyph from headers that previously displayed as mojibake in some Windows hosts.
- Preset parameter names, IDs and defaults remain unchanged; existing XT sessions should retain their values.

## Installation

1. Back up your working v0.3 `.vst3` bundle and current preset.
2. Upload the contents of this directory to your existing GitHub repo root.
3. Run **Build SmoothRide VST3 (Windows)** under Actions.
4. Download the artifact and replace the installed bundle while audio hosts are closed.
5. In XT, confirm your saved v0.3 parameter values and play music; compare its sound with v0.3.

## Diagnostic hints

- Peak meters changing with `BELOW FLOOR` means the plugin is receiving audio but the 250 ms RMS detector is below the set floor.
- `MAX BOOST` means the requested gain has hit the configured upper bound. The *applied* ride may still be catching up due to Gain Up smoothing.
- A 0 dB history can be normal if the signal is below the floor or the gain request is near 0 dB.
- The peak reduction display is sample-peak attenuation, not a true-peak limiter.

## Test status

The standalone DSP regression tests can be compiled with a regular C++17 compiler, for example:

```bash
g++ -std=c++17 -O2 tests/dsp_test.cpp -o /tmp/smoothride-tests && /tmp/smoothride-tests
```

The Windows JUCE GUI requires the supplied GitHub Actions workflow and has not been compiled locally in this environment.
