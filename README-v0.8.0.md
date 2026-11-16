# SmoothRide v0.8.0 — Usability and Release Polish

Windows x64 VST3 built with JUCE 8.0.12. This is a source package; GitHub Actions compiles and validates the plugin.

## Improvements

- Listening view shows a visually distinct **SAVED**, **MODIFIED**, or **CUSTOM** preset-status chip. The chip describes the *loaded* configuration, never just the user-preset dropdown selection.
- The loaded preset name and any different dropdown selection are shown separately.
- Factory preset selector is labeled **FACTORY PRESET**. Factory selections apply immediately; user-preset selections require **Load**.
- Save, Load, and Delete tooltips clarify that **Save** never overwrites a preset and **Update** is the explicit overwrite action.
- Added JUCE-independent `PresetWorkflowPolicy.h` and `tests/preset_workflow_test.cpp`; this logic is shared by the processor's preset-save and modified-detection paths.
- The saved preset-name limit still uses JUCE character counts (not UTF-8 byte length); previously saved Unicode names remain valid.
- The 1080×600 editor and all existing audio behavior are unchanged.

## Smoke-test checklist

1. Load a saved preset: observe **SAVED** and the correct name.
2. Change Mix: observe **MODIFIED**. Click **Revert**: observe **SAVED**.
3. Change Mix, click **Update**, close and reopen the editor: active preset should remain **SAVED**.
4. Select a *different* user preset without loading: active preset must not change; select **Load** to apply.
5. Create a new named preset with **Save**. Reusing that name should not overwrite it.
6. Import/export presets and use A/B switching. Confirm smooth playback with Adaptive Floor.
7. Check that the Listen/Advanced views work and the plugin window stays fixed size in EqualizerAPO-XT.

## Automated checks

The GitHub Actions workflow builds `SmoothRidePresetWorkflowTest` and runs it with CTest, alongside the existing DSP, performance, and parameter-cache regression suites. This standalone test validates shared name and modified-state policies; the complete JUCE editor and host state are checked by the Windows build and the manual XT smoke tests above.

The source package has not been compiled as a VST3 in this environment. Keep the tested v0.7.4 build as a rollback until GitHub Actions and XT testing succeed.
