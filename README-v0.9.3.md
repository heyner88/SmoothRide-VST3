# SmoothRide v0.9.3 — stereo readouts and UI alignment

Build from this full source package with the included GitHub Actions workflow (JUCE 8.0.12).

## Changes since v0.9.2 arc fix
- Stereo input/output meter panels now show independent **L and R dBFS readings** rather than the maximum of the two channels. The existing channel-peak telemetry is reused without changes to the DSP path.
- Factory selector placeholder shortened to **Select factory preset**.
- Factory selector follows the **currently active factory preset**, returning to its placeholder when a user preset is loaded, instead of displaying a stale selection.
- Advanced View status notifications now remain visible in a dedicated lower-right area; hover for the full message.
- Fixed 1080×600 window retained. All preset IDs, file formats and APVTS parameters retained.

**No processor, GainRider or DSP telemetry changes.**

## Validation
Independent C++17 regression tests can be run via `g++ -std=c++17 -O2 tests/<test>.cpp -o <binary>`.
The native Windows JUCE/VST3 build and GUI rendering must be checked with GitHub Actions and EqualizerAPO-XT.
