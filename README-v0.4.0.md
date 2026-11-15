# SmoothRide v0.4.0 — Performance + dual-history telemetry

This release is **Phase 1** of the v0.4 roadmap. It preserves the v0.3.3 gain-rider DSP and all 11 parameter IDs/defaults, while adding the following instrumentation:

- DSP AVG: exponentially-smoothed *audio callback execution time divided by the available audio block duration*, as a percentage.
- PEAK: maximum observed callback load since `prepareToPlay`.
- LAST: duration of the most recently processed block, in microseconds.
- LATENCY: reported plugin latency in samples (0: no intentional lookahead buffering in current implementation).
- Dual history: gain-riding (mint, +/- 12 dB on its own scale) and detector RMS (blue, -100..0 dBFS, independently scaled).

**Performance numbers are not the same as Windows process CPU percentages.** They exclude host overhead, rendering, other effects and OS scheduling. A callback over 100% of its block budget is a potential real-time deadline concern; a percentage under 100% alone cannot guarantee glitch-free audio. Peaks reset when the host calls `prepareToPlay`. A measurement timer also incurs small overhead.

### What did not change

- `Source/GainRider.h` is byte-for-byte unchanged from v0.3.3.
- Existing plugin parameter IDs, ranges and defaults are unchanged.
- JUCE VST3 product name and bundle ID are unchanged for replacement in XT.
- GUI remains fixed at 1080 x 600 to avoid the previous window disappearance issue.

### Build

Upload the *contents* of this directory to the repo root and use your existing `.github/workflows/build-windows.yml` workflow. The workflow downloads JUCE 8.0.12 and builds the Windows VST3; no compiler is needed locally.

Before installing: back up `SmoothRide.vst3`, close all instances of the plugin and host editor, and restore your preferred parameter state after replacement if needed.

### Tests

Standalone tests on Linux (no JUCE dependency):

```sh
g++ -std=c++17 -O2 tests/dsp_test.cpp -o /tmp/smoothride_dsp_test && /tmp/smoothride_dsp_test
g++ -std=c++17 -O2 tests/performance_test.cpp -o /tmp/smoothride_perf_test && /tmp/smoothride_perf_test
```

The new JUCE UI has not been compiled or tested on Windows in the source-generation environment; the GitHub Actions run and XT installation remain necessary validation steps.

### Planned for future v0.4.x increments

Named persistent user presets and an optional level-matched A/B control. Those are deliberately not included in this phase to limit regression risk.
