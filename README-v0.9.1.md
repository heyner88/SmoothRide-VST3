# SmoothRide v0.9.1 — interface refinement

Based on the tested v0.9.0 source. No changes to the audio processor, gain-riding DSP, parameter IDs, preset format, or 1080 x 600 editor size.

## Changes

- Fixed duplicate FACTORY preset label in the bottom bar.
- Replaced the Soft Floor and Adaptive Floor 0/1 knobs with single-click, clearly labeled ON/OFF TextButtons connected through JUCE AudioProcessorValueTreeState ButtonAttachment.
- Retained host automation, preset saving/loading and the same parameter IDs.
- Increased prominence of live gain and rider status values.
- Kept advanced view, preset/A-B functionality and Noise Guard controls.

## Manual Windows XT testing

1. Confirm only one FACTORY label appears; no overlap in the footer.
2. Click Soft Floor and Adaptive; each switches immediately and survives closing/reopening the editor and a host restart.
3. Load a preset and verify both toggles follow its saved values. Test revert and A/B recall.
4. Check knob drag and double-click reset on continuous parameters.
5. Confirm music and livestream playback remain smooth.

Source tests can run without JUCE; full plugin build and visual validation require Windows GitHub Actions/XT.
