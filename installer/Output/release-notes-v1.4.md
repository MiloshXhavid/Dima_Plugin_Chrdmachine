## Dimea Chord Joystick MK2 v1.4

Dimea Chord Joystick MK2 v1.4 adds a full dual-axis LFO system, a beat clock indicator, and gamepad preset navigation — the biggest update since launch. All features ship as a single installer with no breaking changes to existing presets.

### New in v1.4

**LFO System**
- Dual-axis LFO independently controls X and Y joystick modulation
- 7 waveforms: Sine, Triangle, Saw Up, Saw Down, Square, Sample & Hold, Random
- Sync mode locks LFO phase to DAW BPM; Free mode runs at any rate (0.01–20 Hz)
- Per-LFO controls: Phase shift (0–360°), Distortion (additive jitter), Level (depth)

**Beat Clock**
- Flashing dot on the Free BPM knob pulses once per beat
- Follows DAW transport when DAW sync is active; follows Free BPM otherwise

**Gamepad Preset Control**
- Option / Guide button toggles preset-scroll mode
- In preset-scroll mode, D-pad Up/Down send MIDI Program Change +1 / −1
- Plugin UI shows "OPTION" indicator when preset-scroll mode is active

### Install

1. Download `DimeaChordJoystickMK2-v1.4-Setup.exe`
2. Run the installer — Windows SmartScreen will show a security warning because this binary is unsigned. Click **More info** → **Run anyway** to proceed.
3. Admin privileges required (installs to the system VST3 folder)
4. Rescan plugins in your DAW

### Requirements

- Windows 10/11 64-bit
- VST3-compatible DAW (Ableton Live, REAPER, etc.)
- Xbox / PS4 / PS5 controller optional (for gamepad input)
