# SmoothRide v0.6.0 — Experimental adaptive floor

Based on v0.5.1. The existing manual floor remains the default (Adaptive Floor = 0). Toggle the twelfth rotary control in the bottom-right card to 1 to enable adaptive floor. The status line shows the effective floor in real time.

## Behavior

- Uses detector RMS to lower the effective gate floor as program level decreases. The effective floor can drop as low as -85 dBFS, never higher than the manually set floor. The threshold is normally 8 dB below the current detector level.
- Hard silence cutoff at -88 dBFS detector RMS; at or below this level, adaptive riding is disabled. This is independent of the manual floor setting.
- Keeps all gain caps, attack/release, peak ceiling, and dry/wet processing unchanged.
- **Important limitation:** this is level-based detection, NOT speech recognition or noise classification. Noise or hiss above -88 dBFS may still be boosted. Test with quiet livestreams and room/noise-only segments before relying on it.
- Manual mode remains the v0.5.1 DSP path.

## Preset compatibility

- Portable v0.6.0 preset files use version 2 and store Adaptive Floor; files exported by v0.5.x (version 1) still import with Adaptive Floor OFF.
- New host-state parameter is OFF by default. Make a backup of your v0.5.1 host settings before upgrading.

## Acceptance test

1. At -60 dB manual floor, play a quiet stream (around -70 to -80 dBFS detector RMS) with adaptive OFF; confirm BELOW FLOOR.
2. Set ADAPTIVE FLOOR to 1 and watch Effective Floor and Rider Status. Try speech/music and a quiet/noise-only segment.
3. Verify gain returns smoothly when the stream stops; check no crackle, distortion or unwanted noise boost.
4. Re-check user preset import/export, A/B and full XT restart persistence.

The additional adaptive gate tests can be run without JUCE using g++ -std=c++17 tests/dsp_test.cpp -o dsp_test && ./dsp_test. Windows JUCE compilation and XT listening test remain to be performed.
