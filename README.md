# Arcade Chord Control

Transform your Game Controller into a powerful MIDI Instrument designed for harmonic chordgeneration. Game your Softsynth or analog Synthesizer with your Controller! 

A MIDI chord Generator VST3/AU plugin for Windows and macOS. Plug in any gamepad or use the on-screen joystick — move it around and play chords in any scale, key, and voicing across a x/y matrix.

---

## Download

**[Latest release — v1.9.1 Beta](https://github.com/MiloshXhavid/Dimea_Arcade_Chord_Control/releases/latest)**

| Platform | File | Install |
|----------|------|---------|
| **Windows** | `DimeaArcade-ChordControl-v1.9.1-Beta-Setup.exe` | Run installer, rescan plugins in DAW |
| **macOS** | `Arcade-Chord-Control-Beta-Test-macOS-v1.9.1-Beta.zip` | Unzip, copy to plugin folders (see below) |

**Windows requirements**
- Windows 10 or 11, 64-bit
- VST3-compatible DAW (Ableton Live, REAPER, FL Studio, Bitwig, etc.)
- Xbox, PS4, or PS5 controller — optional but recommended

**macOS requirements**
- macOS 11.0 (Big Sur) or later
- Apple Silicon (arm64) and Intel (x86_64) — universal binary
- VST3 or AU compatible DAW (Logic Pro, Ableton Live, Reaper, etc.)
- Xbox, PS4, or PS5 controller — optional but recommended

---

## macOS Install

1. Unzip the downloaded file
2. Copy `Arcade Chord Control Beta-Test.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
3. Copy `Arcade Chord Control Beta-Test.component` → `~/Library/Audio/Plug-Ins/Components/`
4. Restart your DAW

### "Apple can't check for malware" — Fix

macOS blocks unnotarized plugins by default. Do one of the following:

**Option A — System Settings (easiest):**
Go to **System Settings → Privacy & Security → scroll down → Allow Anyway**

**Option B — Terminal (recommended):**
```bash
sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Arcade Chord Control Beta-Test.vst3" "/Library/Audio/Plug-Ins/Components/Arcade Chord Control Beta-Test.component" 2>/dev/null; sudo xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Arcade Chord Control Beta-Test.vst3" ~/Library/Audio/Plug-Ins/Components/"Arcade Chord Control Beta-Test.component" 2>/dev/null
```
Enter your Mac password when prompted, then restart your DAW.

**Option C — If your DAW still refuses to load the plugin after the steps above:**
```bash
sudo codesign --force --deep --sign - "/Library/Audio/Plug-Ins/VST3/Arcade Chord Control Beta-Test.vst3"
```
This applies an ad-hoc signature, which some DAWs require even after quarantine is removed. Enter your Mac password when prompted, then rescan plugins in your DAW.

This warning is expected for beta releases. Code signing will be added in a future release.

---

## What it does

Transform your Game Controller into a powerful MIDI Instrument! Move the right Joystick (or analog stick on a gamepad) and sellect a Chordstructure,then play the Notes individualy as you please. Per default the Y axis controls the root note and the third, the X axis controls the fifth and the tension. Every position snaps to the scale you choose, so everything stays in key automatically. Control CC MIDI Modulation with the left Joystick or with the integrated LFO Section. Go from gentle to using it as a selfgenetative Granular Engine. Internal looper, Triggerselection, CONTROLED Randomness, 8 Step Arpeggiator style Sequencer, X and Y lane Modulation and two crossmodulatable LFO's with Pitch or CC Modulation target.

The four chord voices go out on separate MIDI channels so you can route them to different instruments or apply different effects per voice. Or select Single Channel to play an external Synthesizer via one selectable Channel.

> THIS IS A MIDI INSTUMENT AND DOES NOT PRODUCE SOUND ON ITS OWN.
This Plugin controls a MIDI capable soundsource like every DAW instrument does or any physical Instrument with MIDI capabilities.

> Place the plugin on a MIDI track in your DAW, before your instrument. In Ableton Live: add it on a MIDI track, then route that track's output to an instrument track.

> (Manual included)


---

## Features

### Chord engine
- Y axis → root + third, X axis → fifth + tension
- 20 built-in scales (major, minor, modes, pentatonic, blues, etc.) plus a custom scale editor
- Global key/transpose, per-voice octave offsets, per-voice interval adjustments
- 4 voices on MIDI channels 1–4 (configurable per voice)
- **Smart Chord Display** — infers correct chord quality (Cm7, Gmaj7#11, sus4, etc.) from the active scale in real time; retains last name during silence

### Triggers
- **Touchplate** — 4 pads, one per voice, velocity sensitive
- **Joystick** — trigger all voices when the stick leaves center
- **Random** — generative rhythmic triggers with density, subdivision, and gate controls per voice
- Hold mode per voice

### LFO modulation
- Dual LFOs (X and Y) with oscilloscope display
- Shapes: sine, triangle, saw, ramp, square, S&H
- Targets: LFO frequency, phase, level, Gate, or any of 18 MIDI CCs
- **Sister LFO cross-modulation** — route X into Y or Y into X with a bipolar attenuator (−1…+1)
- SYNC (DAW tempo) and JOY (joystick-driven) modes
- REC lane — record one full loop cycle of LFO movement; CLR to clear

### Looper
- Record and loop chord progressions in sync with your DAW
- Record Gates / Joystick / Mod Whl lanes independently
- Adjustable time signature (3/4, 4/4, 5/4, 7/8, 9/8, 11/8) and length (1–16 bars)
- **Trigger quantization** — snap recorded MIDI to grid: Off / Live / Post, subdivisions 1/4–1/32
- Rec Lane Undo — press a lit Rec button to clear that lane individually
- Looper position bar shows playback progress in real time

### Arpeggiator
- 8-step grid (ON / TIE / OFF per cell) for melodic patterns
- LEN control (1–8 active steps); RND SYNC: FREE / INT / DAW
- Rate, order, and gate% controls

### Gamepad input
- Right stick → joystick pitch
- Left stick X/Y → configurable CC targets (default CC74 cutoff + CC71 resonance) with attenuation
- L1/L2/R1/R2 → trigger individual voices; L3 → all voices
- X → looper play/stop, Square → reset, Triangle → record, Circle → delete
- Status label shows detected controller type: PS4 / PS5 / Xbox / No controller
- **MIDI Panic** button — sends all-notes-off on all 16 MIDI channels instantly

### Custom CC routing
- LFO CC Dest and Left Stick X/Y Mode dropdowns support any CC 0–127
- Custom assignments persist in presets

### Visual
- **Resizable window** — drag corner to scale 0.75×–1.0×; aspect ratio locked; persists across save/load
- **Mini mode (◱)** / **Maxi mode (⛶)** — pad-only or pad-fills-display
- **Pitch Axis Crosshair** — live note names at joystick position; right-click pad to toggle
- Living Space Starfield — parallax layers, shooting stars, nebula blobs; crossfades with warp
- Scale keyboard display
- Velocity-sensitive knob drag (slow = fine, fast = broad sweep)

---

## Windows Installation

1. Download `DimeaArcade-ChordControl-v1.9.1-Beta-Setup.exe` from the [releases page](https://github.com/MiloshXhavid/Dimea_Arcade_Chord_Control/releases/latest)
2. Run it — Windows SmartScreen may warn on first run, click "More info → Run anyway"
3. Admin privileges required (installs to `C:\Program Files\Common Files\VST3\`)
4. Open your DAW and rescan VST3 plugins
5. Add **Arcade Chord Control** on a MIDI track, before your instrument

---

## Building from source

**Windows:**

Requires: CMake 3.22+, Visual Studio 2022, Windows SDK

```bash
git clone https://github.com/MiloshXhavid/Dimea_Arcade_Chord_Control.git
cd Dimea_Arcade_Chord_Control
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target ChordJoystick_VST3
```

**macOS:**

Requires: CMake 3.22+, Xcode 14+, macOS 11.0+

```bash
git clone https://github.com/MiloshXhavid/Dimea_Arcade_Chord_Control.git
cd Dimea_Arcade_Chord_Control
cmake -G Xcode -B build-mac -S . -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
xcodebuild -project build-mac/ChordJoystick.xcodeproj -scheme ChordJoystick_All -configuration Release ONLY_ACTIVE_ARCH=NO
```

---

## Changelog

### v1.9 — Living Interface
- Resizable window (0.75×–1.0×), mini/maxi modes
- Pitch Axis Crosshair with live note names
- Smart chord display — correct quality from scale, retained during silence
- Living Space Starfield with parallax, shooting stars, nebula
- Arpeggiator step pattern grid (8-step, ON/TIE/OFF)
- Sister LFO attenuation slider (−1…+1 bipolar)
- Custom CC routing for LFO and left stick targets
- Rec Lane Undo; velocity-sensitive knob drag
- **macOS support** — universal binary (Apple Silicon + Intel), VST3 + AU

### v1.8
- LFO engine — dual LFOs with oscilloscope, 6 shapes, DAW sync, JOY mode
- Sister LFO cross-modulation (X↔Y)
- LFO REC lane (one-cycle capture, CLR)
- Plugin type changed to MIDI effect (place on MIDI track before instrument)

### v1.7
- Filter CC attenuation knob with live update
- LFO panel redesign (oscilloscope, rows layout)
- MOD FIX display and range fixes

### v1.6
- Looper reset phase fixes
- Population knob
- Left stick deadzone
- CC X/Y modes
- Arpeggiator step-counting fix
- Gate length for joystick triggers

### v1.5
- SWAP/INV gamepad routing
- UI polish

### v1.4
- Scale keyboard display
- Per-voice MIDI channel configuration

### v1.3
- LOOPER section panel with rounded border
- Gamepad status label (PS4 / PS5 / Xbox)
- Trigger quantization (Off / Live / Post, 1/4–1/32)
- MIDI Panic button

### v1.2
- DAW sync looper
- Trigger source per voice (pad / joystick / random)
- Random generative triggers with BPM sync

### v1.1
- Gamepad support (Xbox, PS4, PS5)
- Filter mod via left stick → CC74/CC71
- Arpeggiator

### v1.0
- Initial release

---

Made by Dimea Arcade
