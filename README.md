# SmoothRide v0.7.2

See [README-v0.7.2.md](README-v0.7.2.md) for changes and testing.

# SmoothRide v0.6.8

See [README-v0.6.8.md](README-v0.6.8.md) for features, limitations, compatibility and test procedure.

# SmoothRide v0.5.1

Start with [README-v0.5.1.md](README-v0.5.1.md). This project is built in GitHub Actions on Windows.

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


## v0.5.1 UI polish

- Reduced parameter card height slightly and lowered the preset/preset-management rows a few pixels to prevent the preset selector from overlapping the bottom parameter cards.
- Kept the compact 1080 x 600 fixed-size editor and left the DSP unchanged.

## v0.6.8 DSP monitoring

- `Reset DSP` requests an audio-thread-safe reset of AVG, historical PEAK, recent 60-second PEAK and spike count. A new audio block must run for the display to update.
- `60s` tracks the maximum callback budget percentage recorded over the last 60 seconds of **monotonic wall time** (the plugin updates the display when new audio callbacks arrive).
- `SPIKES` counts callbacks whose processing-time budget reached the selected threshold (5%, 10%, 20%, or 40%). Default 10%. The counter is since last monitoring reset.
- Monitoring is in-memory only; the optional spike threshold is not part of presets and defaults to 10% at plugin initialization.
- The gain-riding audio algorithm, parameter IDs and saved preset format are unchanged.


See README-v0.6.8.md for the telemetry consistency fix.


## v0.6.8 toggle timing diagnostics
The UI now displays GUARD TOGGLES, BLOCK % (whole callback cost during last toggle), and RIDER % (time spent in GainRider during that callback). These are wall-clock measures and can include OS preemption; they do not definitively attribute a spike to CPU work. Reset DSP clears these fields. The audio behavior is unchanged.


## v0.6.8 callback timing

On Noise Guard toggle, the telemetry row shows callback **BLOCK**, **PRE**, **RIDER**, and **POST** percentages of the block time budget. PRE includes settings and input analysis; POST includes compensation, output metering, and atomic metrics. MON is telemetry bookkeeping time (microseconds) from the preceding audio callback, which occurs after the BLOCK timer stops. All are wall-clock elapsed times, including any OS preemption; they do not directly measure CPU usage. Reset DSP clears the metrics. No DSP algorithm changes.


See [README-v0.6.8.md](README-v0.6.8.md) for PRE-stage timing diagnostics and test instructions.


## v0.6.8 parameter caching
See [README-v0.6.8.md](README-v0.6.8.md).


See [README-v0.7.2.md](README-v0.7.2.md) for correlated peak/spike callback IDs and test plan.


## v0.7.2 listening view

LISTEN VIEW is on by default. The interface retains meters, status, graph, all controls, and presets while hiding verbose performance readouts and Reset DSP/spike threshold controls. Uncheck LISTEN VIEW to access the existing advanced UI. The selected preset does not load until Load is clicked. This setting is an editor-only preference (defaults to ON when editor is reopened); it does not affect DSP.
