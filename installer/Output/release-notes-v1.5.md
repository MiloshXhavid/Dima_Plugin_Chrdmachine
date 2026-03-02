## DIMEA CHORD JOYSTICK MK2 v1.5

DIMEA CHORD JOYSTICK MK2 v1.5 is the most comprehensive update yet — adding single-channel MIDI routing, per-voice sub-octave doubling, a 6-mode arpeggiator, LFO recording, expanded random trigger modes, left joystick modulation targets, and Gamepad Option Mode 1 face-button dispatch. All features are backward-compatible with existing presets.

### New in v1.5

**Routing**
- Single-Channel mode: all voices → one user-selected MIDI channel (default is Multi-Channel, voices on ch 1–4)
- Reference-counted note collisions in Single-Channel mode: note-off only fires when the last voice on a pitch releases
- Sub-Octave per voice: toggle on any pad to fire a second note-on exactly 12 semitones below on every trigger

**Arpeggiator**
- Steps through the 4-voice chord at configurable Rate (1/4, 1/8, 1/16, 1/32) and Order (Up, Down, Up-Down, Random)
- Unified Gate Length parameter shared with Random trigger source
- Bar-boundary launch: when DAW sync is active, Arp waits for the next bar before stepping begins

**LFO Recording**
- Arm button lets you record one loop cycle of LFO output into a ring buffer
- After one cycle, LFO switches to playback — replaying stored values in sync with the looper
- Distortion stays adjustable during playback; Shape/Freq/Phase/Level/Sync are locked
- Clear button returns to normal LFO mode

**Expression**
- Random trigger sources split into Random Free (continuous) and Random Hold (fires only while pad is held)
- Population knob (1–64) and Probability knob (0–100%) replace the old RND Density/Gate controls
- Subdivision options extended to include 1/64
- Left Joystick X and Y can target: Filter Cutoff, Filter Resonance, LFO Freq, LFO Phase, LFO Level, or Gate Length
- LFO sliders visually track the joystick in real time; releasing the stick returns to the user-set base value

**Gamepad — Option Mode 1**
- Circle: toggle Arp on/off (resets looper to beat 0 on turn-on; arms if DAW sync active)
- Triangle: step Arp Rate up (1 press = step up; 2 quick = step down)
- Square: step Arp Order (same two-press architecture)
- X: toggle RND Sync
- Holding pad button (R1/R2/L1/L2) + R3: toggle Sub-Octave for that voice (any mode)

**UI / Visual**
- Joystick hold-glow particle system — particles animate around the pad on hold
- Mouse drag on the controller illustration controls filter (X) and pitch (Y) directly
- LFO sliders anchor correctly when joystick offset is applied
- Red/blue LFO channel tinting on knobs; MOD FIX knob visual corrected
- Logo and chord display repositioned to middle column for better balance
- Looper panel uses LFO-style box; arp/pad layout aligned to scale top
- Battery icon shows gamepad charge level
- Logo fade-in on load; DIMEA logo scales to fill panel without distortion
- LFO CC target is now a dropdown on both X and Y panels (was hardcoded)

**Bug Fixes**
- Fixed: Ableton MIDI port startup crash when plugin is present in default template
- Fixed: Scale keyboard transpose display showing wrong note name
- Fixed: LFO joystick modulation continuing after stick released
- Fixed: PS button triggers clean soft-disconnect/reconnect instead of crash path
- Fixed: Chord name display blanking on edge-case voicings
- Fixed: Panic now hard-cuts stuck notes from all Random/Hold mode switches
- Fixed: RandomHold touchplate grayout not updating on mode change
- Fixed: Mode-switch now closes all open gates universally
- Fixed: DAW transport stop leaks looper-started notes
- Fixed: SDL HIDAPI — improved gamepad detection for previously unrecognized controllers

### Install

1. Download `DIMEA-ChordJoystickMK2-v1.5-Setup.exe`
2. Run the installer — read the welcome screen note about Windows SmartScreen
3. Admin privileges required (installs to the system VST3 folder)
4. Rescan plugins in your DAW

### Requirements

- Windows 10/11 64-bit
- VST3-compatible DAW (Ableton Live, REAPER, etc.)
- Xbox / PS4 / PS5 controller optional (for gamepad input)
