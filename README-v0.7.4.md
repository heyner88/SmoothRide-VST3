# SmoothRide v0.7.4 — Preset workflow improvements

This is a UI and preset-management maintenance release built from v0.7.3. No changes to GainRider, audio processing, the VST3 ID, or the preset format.

## New actions

- **Save:** Creates a new user preset with the typed name. Existing names are rejected to avoid accidental overwrites; use **Update** on an active preset instead.
- **Update:** Overwrites only the currently active **user** preset using the current parameter values. Available when the active user preset has unsaved changes. Does not act on the dropdown selection and does not overwrite factory presets.
- **Revert:** Restores the currently active preset to its saved parameter values. Works for both user and factory presets and discards unsaved changes.
- **Active preset indicator:** Shows `(modified)` if parameters differ from the saved configuration, and clears it after Update or Revert. The active identity persists across editor reopen/host state restoration.

## Manual regression test plan

1. Load an existing user preset; modify MIX; observe `(modified)`.
2. Click Revert; confirm MIX returns and `(modified)` disappears.
3. Modify MIX; click Update; close and reopen the editor. Check name remains and `(modified)` is absent.
4. Load a *different* preset, select another one in the dropdown without loading it, and verify Update cannot overwrite the selected inactive preset.
5. Type an existing preset name and click Save: it should refuse to overwrite; Save with a unique name should work.
6. Load a factory preset, alter a setting, and use Revert; Update is disabled for factory presets.
7. Restart EqualizerAPO-XT and confirm presets and the active-preset description persist as before.
8. Verify A/B switching, import/export, and live audio stability.

Windows compilation and EqualizerAPO-XT manual tests remain pending.
