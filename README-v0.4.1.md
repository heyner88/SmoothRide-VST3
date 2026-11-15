# SmoothRide v0.4.1 — Named Presets + A/B Snapshots

Based on v0.4.0, preserving GainRider.h **byte-for-byte** and the existing 11 AudioProcessorValueTreeState parameter IDs and ranges.

## What's new

- **Named user presets**: enter a name, press **Save**; select it in the *Saved presets* dropdown and press **Load** or **Delete**.
- **A/B snapshots**: **Store A** and **Store B** capture two complete parameter states; **Recall A** and **Recall B** switch between those saved states.
- Both user presets and A/B snapshots are stored in the **VST3 host project/plugin state**, so they survive reopening the plugin when the host saves and restores plugin state.
- No file system access, network calls, locks, or allocations were added to the real-time audio path. Management runs on the GUI thread.
- Fixed 1080 x 600 interface, as in v0.4.0, to avoid the earlier taller-host-window problem.

## Important persistence limitation

User presets are **host-state scoped**, not a machine-wide preset library. In EqualizerAPO-XT, save/apply the plugin configuration so the host serializes the new preset data. Changing/replacing the plugin configuration without restoring its state can remove those presets. Keep a backup of your XT config before updating. A future version can add explicit import/export to files.

## A/B is not yet loudness matched

A/B immediately recalls parameter snapshots using the standard JUCE host-notified parameter API. It is an audition/compare feature, not a true level-matched crossfade or click-free A/B engine. Large output changes may produce abrupt loudness differences; **lower playback volume before comparing** and keep the Output settings similar in A and B. Automatic gain matching can be developed next after validation.

## Build / upgrade

1. Back up the working v0.4.0 VST3 bundle and your XT configuration.
2. Upload this folder's contents to your existing GitHub repository root.
3. Run **Build SmoothRide VST3 (Windows)** in GitHub Actions.
4. Download the artifact from the successful run and replace the installed VST3 bundle while the host/editor is closed.
5. Reopen XT, verify old parameter values, then test preset and A/B state across closing/reopening.

Windows JUCE/VST3 compilation is not available in this environment and has **not** been verified locally. The existing standalone DSP and performance tests were run successfully on Linux.
