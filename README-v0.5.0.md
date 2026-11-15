# SmoothRide v0.5.0 — Reliability and portable presets

Built on the listening-validated v0.4.2 baseline. Core `GainRider.h`, audio algorithm, A/B matching and fixed 1080x600 UI are unchanged.

## Changes

- **Import** / **Export** buttons for a selected custom preset, using `.smoothridepreset` XML files.
- Export includes a portable, versioned snapshot of all 11 parameters (no A/B states or private host metadata).
- Import validates file size (64 KiB max), file type/version, name length, all parameter IDs, numerical values and allowed parameter ranges. Import rejects existing names rather than silently replacing them.
- Adds **CTest regression tests** to Windows CI; failing tests stop the build.
- Adds **pluginval** VST3 validation (strictness level 5) and test reports; validation failure stops the build.
- Produces a versioned Windows VST3 ZIP, with a SHA-256 checksum file. No automatic public release or installer is created.

## Test on EqualizerAPO-XT

1. Back up the working v0.4.2 VST3 and XT settings.
2. Run the GitHub Actions workflow, download the build artifact, extract the nested VST3 ZIP, and install the `SmoothRide.vst3` folder with XT closed.
3. Save a preset named `Streams Test`. Select it and click **Export**. Choose a `.smoothridepreset` filename.
4. Delete `Streams Test` from the plugin, then click **Import** to restore the exported file. Select it and Load it; verify all settings.
5. Close and restart XT and verify the imported preset persists. Try A/B switching with matching on/off and check for clicks.
6. Check the GitHub Actions log for CTest and pluginval passes and the SHA256SUMS.txt matching the downloaded ZIP.

### Notes / limitations

- Import/export works only on saved **user presets**, not A/B calibration or full plugin state.
- A duplicate imported preset name is rejected to avoid unexpected data loss.
- JUCE file dialogs are launched asynchronously from the editor; no file I/O is performed on the audio thread.
- If pluginval finds a problem, preserve the GitHub Actions logs and do not replace v0.4.2 until resolved.
- This source package was not compiled against JUCE on a Windows machine here; the GitHub Actions run is the first full VST3 compile/validation.
