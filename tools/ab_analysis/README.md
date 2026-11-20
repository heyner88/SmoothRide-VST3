# SmoothRide — Offline OutlawCM vs SmoothRide A/B experiment

This is an **optional standalone experiment**. It does not change SmoothRide DSP, presets, VST3 build, or the Windows audio setup.

## Safety and scope
- **Do not commit or upload** `OutlawCM.dll` or any third-party plugin binaries to this public repository.
- The script never loads or executes VST plugins; each render must be made in an audio host capable of VST2 and VST3 (for example, a DAW already available on your computer).
- No microphone, audio loopback, or Windows APO reconfiguration is required.
- Use legally obtained audio; the generator below produces original synthetic test material.

## Procedure
1. On Windows with Python 3 installed, generate the same dry test file for both plugins:
   ```powershell
   py -3 tools/ab_analysis/generate_test_audio.py --out test-input.wav
   ```
2. Load `test-input.wav` into a DAW or offline-rendering audio host.
3. Render **OutlawCM VST2 only** to `outlaw.wav`. Disable all other effects, including DAW master processing; no normalizing, fades, resampling, dithering, or automatic loudness matching.
4. Render **SmoothRide VST3 only** from the same timeline and source to `smoothride.wav`, using your recorded settings. Use the same sample rate, channel count, WAV length, and PCM16 export format. Allow enough time for the rider to react during the test; use the whole 24-second stimulus.
5. Compare:
   ```powershell
   py -3 tools/ab_analysis/compare.py test-input.wav outlaw.wav smoothride.wav --out ab-results
   ```
6. Open `ab-results/gain_comparison.svg` in your browser; inspect `summary.json` and `windows.csv`. These report **windowed dBFS/RMS and relative gain**, not LUFS or perceptual immersion.

For an authentic street stream, replace the generated input with the **same original, legally recorded stereo WAV**, and render the two outputs from it. Match listening loudness independently before making subjective judgments.

### Interpretation
- A higher quiet-window gain may make ambience more audible, but is **not proof** of more immersion.
- A difference in gain recovery after foreground bursts may explain why ambient sounds feel stronger between speech.
- The tool does not estimate LUFS, spectral changes, true peaks, stereo width, or exact gain envelope when processing is nonlinear.
- Silence and near-silence can yield meaningless relative dB gains: those windows are excluded from gain-summary statistics.
- Compare only isolated plugin renders. Any remaining EQ, Sonar, LoudMax, or limiter in the render invalidates a plugin-only comparison.

### Next iteration
For fully automated VST2+VST3 rendering, a Windows plugin-host adapter would be needed and must be tested separately. This first version deliberately avoids coupling experimental hosting code to our stable audio engine.
