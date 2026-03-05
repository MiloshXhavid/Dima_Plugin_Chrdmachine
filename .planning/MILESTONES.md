# Milestones
## v1.7 Space Joystick (Shipped: 2026-03-05)

**Phases completed:** 22 phases, 45 plans, 4 tasks

**Key accomplishments:**
- (none recorded)

---


## v1.5 Routing + Expression (Shipped: 2026-03-02)

**Phases completed:** 15 phases, 35 plans, 4 tasks

**Key accomplishments:**
- (none recorded)

---

## v1.4 LFO + Clock (Shipped: 2026-02-26)

**Phases completed:** Phases 12–16 (5 phases)

**Key accomplishments:**
- Dual LFO engine (X + Y axis) — 7 waveforms (Sine/Tri/Saw↑/Saw↓/Square/S&H/Random), Freq/Phase/Distortion/Level, DAW-sync
- LFO UI section left of joystick — shape dropdown, per-axis on/off toggle, Sync button
- Beat clock dot on Free BPM knob face — flashes on beat, follows DAW sync, 30 Hz polling
- Gamepad Preset Control — Option mode: D-pad Up/Down scrolls Program Changes, OPTION indicator
- MIDI Panic (CC64+CC120+CC123, 48 events, no CC121), trigger quantization Off/Live/Post, pluginval level 5 re-verified
- GitHub v1.4 release + Inno Setup v1.4 installer

## v1.0 MVP (Shipped: 2026-02-23)

**Phases completed:** 7 phases, 17 plans, 2 tasks

**Key accomplishments:**
- Reproducible JUCE 8.0.4 build with static CRT — no MSVC runtime dependency, loads in Reaper and Ableton
- 26 Catch2 tests passing — ScaleQuantizer, ChordEngine (15 unit tests), LooperEngine (11 stress/boundary tests)
- 4-voice MIDI note-on/off with note-off guaranteed on all exit paths (bypass, releaseResources, transport stop) — DAW-verified in Reaper
- Per-voice trigger sources: TouchPlate, JOY retrigger (scale-degree boundary model), DAW-synced random gate with density 1–8 hits/bar
- Lock-free LooperEngine (AbstractFifo double-buffer) + Ableton MIDI effect fix — plugin loads correctly on MIDI tracks in Ableton 11
- SDL2 PS4/Xbox gamepad integration — process-level singleton, CC74/CC71 gating, disconnect all-notes-off, full DAW verification
- pluginval strictness level 5 PASSED (19/19 suites) + Inno Setup 3.5 MB installer clean-machine approved — v1.0 release candidate ready

---

