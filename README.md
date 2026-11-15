# SmoothRide v0.5.0

Start with [README-v0.5.0.md](README-v0.5.0.md). This project is built in GitHub Actions on Windows.

# SmoothRide VST3 v0.3.1

See `README-v0.3.1.md` for changes and build instructions.

# SmoothRide v0.3 — live monitoring and custom UI

This is an **experimental VST3 source release**, not an installed binary. The plugin is intended for EqualizerAPO-XT on Windows x64.

## Changes in v0.3

- Original v0.2 gain riding engine **unchanged** (`Source/GainRider.h` byte-for-byte identical).
- Custom JUCE editor with nine parameter knobs, linked via `AudioProcessorValueTreeState` attachments.
- Live input and output **sample peak** meters (dBFS), current gain-riding adjustment (dB), and instantaneous sample-peak safety attenuation.
- Gain history of the last approximately 24 seconds **while the editor is open** (10 readings per second). History is not saved.
- Optional starting presets: v0.2 Baseline, Music (subtle), Streams (controlled), Movies (wider dynamics). Choosing a preset changes parameter values; no automatic content detection.
- The plugin continues to save its nine parameters in host state and uses the **same parameter identifiers** as v0.2 for compatibility.

### Important limitations

- Input/output meters use audio-block sample peaks, **not true peaks** and not LUFS; the meters may miss momentary peaks between editor refreshes.
- The safety ceiling is a zero-lookahead **sample-peak clamp** in v0.2 DSP, not a true-peak brickwall limiter. Keep your preferred final limiter if appropriate.
- Meters and graph are observational only, not part of the audio DSP.
- SmoothRide is not a vocal isolator; it rides overall program loudness. Presets are experimental.
- Replacing a VST3 bundle while loaded can cause crashes. Close all audio apps/XT editors first, back up v0.2, and restart audio applications after installation.
- Compiled with a JUCE 8 dependency downloaded in GitHub Actions; JUCE licensing conditions apply for redistribution.

## Build with GitHub Actions

1. In your existing repository, replace the `Source/` folder, `CMakeLists.txt`, and this README. Keep `.github/workflows/build-windows.yml` in the repository root.
2. Make sure the project root directly contains `CMakeLists.txt`; do not upload the enclosing `SmoothRideVST3-v0.3` folder as another nesting level.
3. Commit changes to `main`. Navigate to **Actions > Build SmoothRide VST3 (Windows)** and inspect the run.
4. On success, download the `SmoothRide-Windows-x64-VST3-...` artifact, extract it, and copy the complete `.vst3` bundle to `C:\Program Files\Common Files\VST3\`.
5. Test at reduced Windows volume and confirm your v0.2 settings before using presets.

The code has not been compiled with JUCE here; the standalone DSP regression tests pass. The GitHub Actions Windows compilation is the next validation step.


See README-v0.3.2.md for changes and compatibility.


See README-v0.3.3.md for compact editor changes.


See **README-v0.4.2.md** for comparison features, calibration, and testing.
