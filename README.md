# SmoothRide v0.1 — original VST3 general-purpose stereo gain rider

**Prototype, not a tested Windows binary.** Made for EqualizerAPO-XT testing. No W.A. Production code or assets used.

## Goals
A musical, slow RMS gain rider with *bounded* gain adjustments, stereo linking, automatic silence detection, dry/wet blend and output make-up gain. Default controls resemble the workflow you liked in OutlawCM, but the algorithm is original and **will not sound identical**.

| Parameter | Default | Meaning |
|---|---:|---|
| Target RMS | -15 dB | Relative gain target, not LUFS |
| Floor | -60 dB | Don't increase silence/background noise |
| Range | +/-6 dB | Maximum internal gain ride |
| Speed | 1800 ms | Ride smoothing time constant |
| Mix | 65% | Blend ridden signal with dry signal |
| Output | +6 dB | Final makeup, before sample peak safety |
| Safety ceiling | -1 dBFS | Stereo-linked sample-peak protection (NOT true peak) |

## Windows build
Requirements: Visual Studio 2022 Desktop development with C++, Git, CMake 3.22+, internet to download JUCE. JUCE 8 uses AGPLv3 or a commercial license; review licensing before distribution of a JUCE-based plugin.

In PowerShell, from this directory:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build-windows.ps1
```

Or manually:

```powershell
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target SmoothRide_VST3
```

Locate `SmoothRide.vst3` under `build/SmoothRide_artefacts/Release/VST3/` and copy the **whole VST3 bundle** into `C:\Program Files\Common Files\VST3\` (admin privileges may be required). Restart or rescan EqualizerAPO-XT. The editor uses JUCE's generic parameter UI for now.

## Safe first test
1. Backup your XT config and turn down Windows output volume.
2. Disable all other riders, normalizers and compressors. Keep LoudMax bypassed for the initial test.
3. Add SmoothRide after EQ/MSED and before any optional final limiter.
4. Start with **Output 0 dB, Mix 50%, Range +/-3 dB**, Target -15 dB, Floor -60 dB, Speed 1800 ms, Ceiling -1 dBFS.
5. Compare bypass and enabled on a known music track. Gradually increase Output only as needed; monitor peak safety limiting and listen for pumping.
6. Save your original OutlawCM settings so you can roll back easily.

## Current limitations
- Native Windows/XT host compatibility **not yet verified**; build and live tests are required.
- No true-peak/intersample limiting; this is a zero-lookahead sample-peak ceiling, which can distort transients. External true-peak limiter recommended for critical work.
- No LUFS metering, no UI gain-reduction meter, no per-track normalization.
- Stereo/mono only, RMS detector fixed at 250 ms. Startup levels may shift until the detector settles.
- Parameters are atomic at block boundaries via JUCE APVTS. No allocation or locking in the DSP sample loop.
- This is a proof of concept, not a drop-in replacement for proprietary Outlaw.

## Standalone DSP smoke test (Linux/macOS/MinGW)

```bash
g++ -std=c++17 -O2 tests/dsp_test.cpp -o dsp_test && ./dsp_test
```

## Build automatically with GitHub Actions

Upload all project files including the hidden `.github/workflows/build-windows.yml` file to a repository with a `main` branch. In GitHub, open **Actions** → **Build SmoothRide VST3 (Windows)** → **Run workflow**. Once the run succeeds, download the `SmoothRide-Windows-x64-VST3` artifact. Extract it and preserve the `.vst3` bundle directory structure when installing into `C:\Program Files\Common Files\VST3\`.

The workflow uses a hosted Windows 2022 runner, Visual Studio 2022, CMake, JUCE 8.0.12 and uploads the VST3 bundle. This workflow and the resulting VST3 have not yet been validated in a GitHub-hosted build or in EqualizerAPO-XT. If the build fails, inspect the Actions logs; it may require source fixes.
