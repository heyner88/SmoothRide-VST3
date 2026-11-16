# SmoothRide v0.7.3 – Persistent active-preset display

- Displays **ACTIVE PRESET: name** in Listening View after loading a saved preset or applying a factory starting preset.
- The active preset name and a parameter reference snapshot are kept in the existing JUCE/APVTS host state, surviving editor close/reopen and host project restoration when the host saves state.
- If any audio parameter is changed from the loaded preset, the label displays **(modified)**. Reverting parameters to the reference values clears that suffix.
- Merely selecting a preset does not mark it active; click Load for user presets. Factory starting presets apply upon selection.
- After editor reopen, preset selectors initialize to the active preset without changing parameters.
- Saving a preset marks the saved configuration active. Deleting the active saved preset clears its name.
- Existing v0.7.2 states without active preset metadata display **Custom settings (no preset loaded)** until a preset is loaded or saved.
- A/B recalls can show the active preset as modified when recalled settings differ; audio processing is unchanged.

Test: load preset, close/reopen editor, change Mix and observe (modified), restore Mix, restart XT with saved state. Windows JUCE compilation pending.
