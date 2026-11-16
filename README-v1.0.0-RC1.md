# SmoothRide v1.0.0 RC1 — Release candidate 1

**Status:** Source release candidate for Windows x64 VST3. Not yet approved as a final stable release.

This RC1 intentionally does **not modify** the proven v0.9.4 audio processor, DSP engine, preset serialization, UI layout, or audio parameters. The only C++ addition is a separate test executable.

## What's new

- Frozen v0.9.4 gain-rider source in `tests/reference/GainRider_v0_9_4.h`.
- `tests/audio_parity_test.cpp`: compares current implementation against that frozen source *sample-for-sample* for 44.1, 48, 96 kHz; mono/stereo; quiet, loud, silence gaps, adaptive floor, soft floor, noise guard, dry and full-wet cases; several processing block lengths.
- Windows CI runs five standalone test executables, pluginval (strictness 5), packages the VST3 and emits SHA256 checksum.

## Important caveats

- Sample parity exercises the standalone gain-rider core; it **does not** compare all samples through the complete JUCE host wrapper or establish backward compatibility of real historical host-state files.
- The existing unit tests and pluginval must still pass on GitHub Actions. Windows compilation, real-host preset/A-B tests, and a long listening session are required before final release.
- Sample-peak limiter is **not** an oversampled true-peak limiter. Experimental Noise Guard remains off by default.
- The version displayed in the interface is updated to `v1.0 RC1` only; gain riding remains unchanged.

## Reference fingerprint

`tests/reference/GainRider_v0_9_4.h` SHA256: `2aafebd10c71b87c391edcc46a0e98bd84c685b0c33f4e6337c622fe44dba61e`

## Running standalone tests on Linux/macOS

```sh
for f in tests/*_test.cpp; do
  g++ -std=c++17 -O2 "$f" -o "/tmp/$(basename "$f" .cpp)" && "/tmp/$(basename "$f" .cpp)"
done
```

## Windows / GitHub Actions

Upload these files to the repository and trigger the included `.github/workflows/build-windows.yml` workflow. Download the versioned VST3 artifact from a successful run. Keep v0.9.4 installed/backed up until RC1 is validated in EqualizerAPO-XT.
