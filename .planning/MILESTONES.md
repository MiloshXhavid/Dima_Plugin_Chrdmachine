# Milestones
## v1.8 Modulation Expansion + Arp/Looper Fixes (Shipped: 2026-03-07)

**Phases completed:** Phases 34, 35, 36, 37, 44 (5 phases, 7 plans)

**Key accomplishments:**
- Cross-LFO modulation: left stick X/Y can target the opposite LFO's Freq, Phase, or Level — 11-item filterXMode/filterYMode with real-time visual slider tracking and Playback guard
- Arp subdivision expansion: arpSubdiv APVTS param extended from 6 → 17 items (4/1 to 1/64T, all triplets) — parity with Random Trigger system
- Arp all trigger sources: removed force-TouchPlate guard — Joystick and Random Free now work with arpeggiator active
- Looper overdub fix: removed internalBeat_=0 double-scan sentinel — fmod absorbs overshoot correctly; TC 14 regression guard added
- Instrument type conversion: IS_MIDI_EFFECT FALSE + IS_SYNTH TRUE + silent stereo output bus — plugin now loads in instrument slot in Ableton, Reaper, Cakewalk, FL Studio
- UAT: 12/12 tests passed; GitHub pre-release v1.8 published; desktop backup created

---

## v1.7 Space Joystick (Shipped: 2026-03-05)

**Phases completed:** Phases 31–33.1 (4 phases including decimal)

**Key accomplishments:**
- Deep-space joystick pad: milky way particle band (3-layer Gaussian, baked), density-driven starfield, semitone grid (in-scale bright / out-of-scale dimmed), BPM-synced breathing glow ring
- Spring-damper cursor physics: inertia while moving, critically-damped spring-back on release (kDamping=0.90, ~150ms, no overshoot)
- Perimeter arc indicator on pad border + note-label compass at cardinal positions
- Gamepad SWAP (left ↔ right stick) + INV (X↔Y axis rotation) routing — applied across all 4 dispatch sites (chord params, filter CC, cursor, LFO)
- SWAP/INV buttons in GamepadDisplayComponent with L3/R3 dynamic labels; cyan/amber lit colors
- LFO waveform oscilloscope in SYNC row (48-sample ring buffer, proportional ±1 scale, blink on positive)
- Left stick deadzone smooth rescaling; CC74/CC71/CC12/CC76 on both stick X and Y axes
- Arp step-counting root cause fixed (start-of-block ppq, backward-jump guard); gate length applies to joystick triggers
- Looper reset syncs all random-free voice phases
- 7 UI bug fixes (Phase 33.1): cursor center on open, critically-damped spring, log-space LFO rate mod, INV axis label/attachment swap, piano black key priority, 3-stripe battery icon
- GitHub v1.7 released as Latest; Inno Setup installer with v1.7 feature text; desktop backup

---

## v1.6 Triplets & Fixes (Shipped: 2026-03-03)

**Phases completed:** Phases 26–30 (5 phases)

**Key accomplishments:**
- Default octave values (Third=4, Fifth=4, Tension=3) + Natural Minor scale as default — correct out-of-box chord voicing
- noteCount_ reference-count clamp removed from all 13 note-off paths — eliminated stuck notes when two voices share a pitch in Single Channel mode
- Triplet subdivisions (1/1T–1/32T) added to both random trigger and quantize selectors; APVTS enum extended
- Random Free redesign — three-branch sync matrix: Poisson stochastic (SYNC OFF), internal free-tempo clock grid (SYNC ON + DAW stopped), DAW ppq grid (DAW Sync ON)
- Looper perimeter bar: clockwise rectangular bar replaces linear strip; ghost ring at idle; LOOPER label always visible; `perimPoint` lambda handles negative-distance wraparound
- GitHub v1.6 release + installer

---

## v1.5 Routing + Expression (Shipped: 2026-03-02)

**Phases completed:** Phases 17–25 (9 phases + 1 decimal)

**Key accomplishments:**
- Single Channel routing mode — all 4 voices → one configurable MIDI channel; note deduplication prevents doubles
- Per-voice Sub Octave — parallel note 1 oct below; Hold/SubOct split button on pad; gamepad R3+pad combo
- Left Joystick X/Y target expansion — LFO freq/phase/level + gate length as modulation targets (6-item dropdowns)
- LFO recording — arm → record 1 loop cycle → playback in sync; Distort stays live; Clear resets to real-time
- Arpeggiator — 6 modes (Up/Down/UpDown/Random/etc.), 4 rates, bar-boundary launch, gate length control
- Random trigger extensions — Free/Hold modes, Population + Probability knobs, 1/64 subdivision
- Option Mode 1 — Circle=Arp toggle, Triangle=Rate, Square=Order, X=RND Sync; R3+pad=Sub Oct
- LFO joystick visual tracking — sliders track joystick at 30 Hz via base+offset model
- Bug fixes: looper anchor drift after record cycle; PS4 BT reconnect crash

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

