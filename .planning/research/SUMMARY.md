# Project Research Summary

**Project:** ChordJoystick v1.5 — Routing + Expression
**Domain:** JUCE VST3 MIDI generator plugin — live performance instrument
**Researched:** 2026-02-28
**Confidence:** HIGH

## Executive Summary

ChordJoystick v1.5 adds six features and two bug fixes to a stable, well-understood codebase. All features are implementable with the locked stack (JUCE 8.0.4, SDL2 2.30.9, C++17) — zero new dependencies are needed. Research confirms that every feature either extends an existing pattern already in the codebase (arp scheduling, APVTS dispatch, note emission) or adds a self-contained subsystem with clear boundaries (LFO recording ring buffer, single-channel reference count). No architectural rewrites are required and no new classes need to be created outside of minor additions to `LfoEngine`.

The recommended build order is strictly dependency-driven: fix the looper start-position bug first (its mis-anchoring would corrupt LFO recording phase if left active), then fix the SDL2 BT reconnect crash (independent; unblocks gamepad testing), then establish single-channel note collision infrastructure (which sub-octave reuses), then sub-octave, then LFO recording, then left-stick target expansion, and finally gamepad Option Mode 1 remapping. The dependency graph is acyclic and each phase is independently compilable.

The key risks are all preventable with specific, well-understood techniques already precedented in the codebase. Note collision on a shared MIDI channel requires a `noteCount_[16][128]` per-pitch reference-count array (audio-thread-only, 2 KB, same pattern as `looperActivePitch_`). Sub-octave orphaned note-offs require a `subOctActivePitch_[4]` snapshot at note-on time. LFO recording double-distortion requires storing pre-distortion samples and applying distortion live during playback. Left-stick modulation expansion and Option Mode 1 RND Sync must use the pending-atomic pattern rather than calling `setValueNotifyingHost` from the audio thread. All have confirmed precedents in the existing codebase or JUCE/MIDI literature.

---

## Key Findings

### Recommended Stack

The full locked stack requires no changes for v1.5. JUCE 8.0.4 provides all required APIs (`AudioPlayHead::PositionInfo`, `MidiBuffer`, `AudioParameterChoice`, atomic wrappers). SDL2 2.30.9 provides the BT fix APIs (`SDL_GameControllerGetJoystick`, `SDL_JoystickInstanceID`). No new JUCE modules and no new CMake targets are required.

**Core technologies (all existing, no additions):**
- **JUCE 8.0.4** — `AudioPlayHead::PositionInfo::getPpqPosition/getBpm/getIsPlaying` already used by LfoEngine and LooperEngine; arp DAW sync extends the same pattern
- **SDL2 2.30.9 (static)** — `SDL_GameControllerGetJoystick` + `SDL_JoystickInstanceID` fix the BT reconnect crash with two guard conditions; no version upgrade needed
- **C++17 / MSVC** — `std::atomic<bool/int/float>`, `std::array<float, 8192>` LFO ring buffer (64 KB, audio-thread-only); all already in use
- **APVTS** — Nine new parameters in `createParameterLayout()` following existing helper pattern; `AudioParameterChoice` list extensions must append-only to preserve saved-state indices for existing users

**Explicit non-additions:** No Ableton Link, no `juce::MPEInstrument`, no `std::map` on audio thread, no SDL3 upgrade, no third-party arpeggiator library, no additional JUCE modules.

See `.planning/research/STACK.md` for full per-feature API analysis and alternatives considered.

### Expected Features

**Must have (table stakes):**
- Single-channel mode: clean toggle (default off), all 4 voices to one MIDI channel with note collision guard preventing stuck notes and doubled-velocity artifacts
- Single-channel note collision: per-pitch reference count (`noteCount_[16][128]`) is the standard MIDI generator approach
- Sub octave: exactly -12 semitones (not tunable), same gate timing as parent voice, persists in state save/load; note-off must match the pitch sent at note-on (snapshot, not recomputed)
- LFO recording: captures exactly 1 loop cycle; Distort parameter stays live and adjustable during playback; shape/freq/phase/level controls grayed out during playback
- Arp gamepad control via Option Mode 1 face buttons (arp on/off, rate, order, RND sync)
- Looper wrong start position bug fix: correctness regression; must be fixed before LFO recording is implemented

**Should have (differentiators):**
- Per-voice sub octave (not global shift): enables bass doubling on root only, leaving third/fifth/tension unaffected — musically more expressive than a global octave offset
- Sub octave + single-channel collision avoidance: prevents mud when a sub-oct note matches another voice on the shared channel
- LFO recording arm/record/play/clear workflow: captures custom LFO shapes from live performance; beat-indexed float ring buffer, 8192 entries
- Left joystick target expansion: LFO freq/shape/level and arp gate len as stick targets beyond CC74/CC71
- R3 + held pad = sub oct toggle for that voice (ergonomic live shortcut)

**Defer (v2+):**
- Sub octave as tunable interval (not fixed -12)
- LFO recording multi-cycle length selection
- D-pad LFO parameter control in Option Mode 1
- SDL3 upgrade
- Global code signing

See `.planning/research/FEATURES.md` for full edge-case analysis per feature, including dependency graph and MVP build order.

### Architecture Approach

All v1.5 features integrate into the existing component structure without new top-level classes. The integration points are: note-emission sites in `processBlock` (steps 14/15/16) for single-channel and sub-octave; `LfoEngine::process()` for the LFO recording state machine; the filter CC section (step 19) for left-stick target expansion; and the face-button dispatch block in step 2 for Option Mode 1 remapping. All key invariants are preserved — no allocation on audio thread, no mutex in processBlock, all cross-thread data via `std::atomic`.

**Modified components (v1.5):**
1. `LfoEngine.h/.cpp` — `RecState` enum + atomic; `recBuf_[8192]`; `arm()`, `clear()`, `getRecState()` API; `process()` state machine (Armed/Recording/Playback)
2. `GamepadInput.h/.cpp` — `rightStickHeld_` atomic + `getRightStickHeld()` accessor (one new member, one timerCallback line)
3. `PluginProcessor.h` — `sentChannel_[4]`, `sentSubOctPitch_[4]`, `looperActiveSubPitch_[4]`, `pendingLStickX_/Y_`, `prevR3WithVoice_[4]`, `effectiveChannel()` inline helper
4. `PluginProcessor.cpp` processBlock — `effectiveChannel()` at all note-on/off sites; sub-oct emission; filter mode new branches; mode-1 face-button dispatch; R3 panic line deletion
5. `PluginEditor.h/.cpp` — single-channel UI; sub-oct buttons per pad; LFO arm/clear/blink UI; filter mode dropdown extensions

**Unmodified components:** `ChordEngine`, `ScaleQuantizer`, `LooperEngine`, `TriggerSystem` (sub-oct and channel remapping happen in the `onNote` callback in processBlock, outside TriggerSystem).

See `.planning/research/ARCHITECTURE.md` for full processBlock execution order, data flow diagram, and per-feature integration analysis.

### Critical Pitfalls

1. **Single-channel note collision** — Two voices on the same channel can compute the same pitch; the first `noteOff` kills both. Prevention: `noteCount_[16][128]` per-pitch reference count (audio-thread-only, 2 KB). Apply to all three emission paths: pad triggers, looper gate, arp. Do not use `std::map` (dynamic allocation prohibited on audio thread).

2. **Sub-octave orphaned note-off** — `heldPitch_[v]` updates every block; computing `noteOff` from the live value at release time misses the pitch that was actually sent if the joystick moved between note-on and note-off. Prevention: `subOctActivePitch_[4]` snapshot at note-on; always use snapshot for note-off. Must also cover looper gate path, reset handler, DAW stop path, and panic flush.

3. **Looper start-position bug poisons LFO recording** — The existing `loopStartPpq_` re-anchor slip (up to one bar after `finaliseRecording()`) produces an audible phase glitch at every LFO loop seam if LFO recording is built before the anchor is fixed. Prevention: fix the anchor bug first by capturing `recordStartPpq_` at recording activation.

4. **LFO recording FIFO overflow** — Routing LFO samples through `LooperEngine`'s `AbstractFifo` (capacity 2048) overflows at approximately 24 seconds and silently discards gate events. Prevention: separate `std::array<float, 8192>` ring buffer inside `LfoEngine`, never routed through `LooperEngine`.

5. **LFO double-distortion on playback** — Recording post-distortion samples then re-applying live distortion compounds noise unpredictably. Prevention: record pre-distortion `evaluateWaveform()` output; apply live distortion on top during playback.

6. **`setValueNotifyingHost` from audio thread** — Left-stick APVTS-param targets and Option Mode 1 RND Sync toggle must not call `setValueNotifyingHost` from processBlock (JUCE assertion, DAW automation flooding, potential deadlock). Prevention: pending `std::atomic<float/bool>` written in processBlock, read and applied in `PluginEditor::timerCallback()` — the established codebase pattern.

7. **SDL2 BT reconnect crash** — Calling `SDL_GameControllerOpen()` inside the SDL event loop before the queue is fully drained opens a handle during active BT enumeration; the handle is invalid on the next axis poll. Prevention: set `pendingReopen_` flag on `SDL_CONTROLLERDEVICEADDED`; open on the next timerCallback tick after queue drain; verify `SDL_GameControllerGetAttached()` immediately after open.

8. **Option Mode 1 Circle fires `looperDelete_` unconditionally** — Face button looper atomics in `GamepadInput` are set regardless of option mode. Prevention: gate the entire face-button looper dispatch in `if (optionMode == 0)`; add separate `arpToggle_`, `arpRateInc_`, etc. atomics for mode 1.

See `.planning/research/PITFALLS.md` for full analysis including 6 additional moderate and minor pitfalls.

---

## Implications for Roadmap

Based on combined research, the dependency graph is acyclic and the correct phase order is unambiguous. The phasing below matches the consensus ordering from FEATURES.md (MVP build order), ARCHITECTURE.md (recommended phase order), and PITFALLS.md (implementation order recommendation).

### Phase 1: Bug Fix — Looper Start Position

**Rationale:** The anchor slip in `loopStartPpq_` after `finaliseRecording()` is a hard prerequisite for LFO recording (Pitfall 5 — phase corruption at loop seam). Fixing it first eliminates the risk of attributing LFO recording phase glitches to the wrong cause. Isolated to `LooperEngine.cpp` with zero risk to other subsystems.
**Delivers:** Correct loop playback alignment after every record cycle; backward PPQ jump detection for DAW loop points.
**Addresses:** Looper wrong-start table-stakes requirement; prerequisite for Phase 5 LFO recording correctness.
**Avoids:** Pitfall 5 (anchor poison); also eliminates a pre-existing user-visible correctness bug independent of v1.5.
**Research flag:** No research phase needed. Add `DBG()` to `anchorToBar()` at implementation time to confirm whether the bug is (a) anchor set at record stop instead of start or (b) off-by-one at bar boundary before writing the fix.

### Phase 2: Bug Fix — SDL2 Bluetooth Reconnect Crash

**Rationale:** Independent of all other phases. Placing it second unblocks reliable gamepad testing for every subsequent feature. Two-file change confined to `GamepadInput.cpp`.
**Delivers:** Stable PS4 BT reconnect; deferred-open pattern with attached verification; instance-ID guard on `SDL_CONTROLLERDEVICEREMOVED`.
**Addresses:** STACK.md Feature 6; PITFALLS.md Pitfall 3.
**Avoids:** Pitfall 3 (invalid handle after rapid re-open; double-close race on fallback poll + event in same tick).
**Research flag:** MEDIUM confidence on exact root cause. The deferred-open + `GetAttached()` verification pattern is safe regardless. If crash persists post-fix, capture `SDL_SetHint("SDL_LOGGING", "all")` event sequence in a debug build.

### Phase 3: Single-Channel Routing Mode

**Rationale:** Foundational routing change. The `noteCount_[16][128]` reference-count infrastructure and `effectiveChannel()` helper established here are reused by sub-octave (Phase 4) and prevent implementing note tracking twice.
**Delivers:** `singleChanMode`/`singleChanTarget` APVTS params; `effectiveChannel()` inline; `sentChannel_[4]` snapshot; `noteCount_[16][128]` reference count applied at all three emission paths (pad, looper, arp); UI toggle + channel selector.
**Addresses:** Single-channel table-stakes feature; collision guard for arp same-pitch adjacent steps.
**Avoids:** Pitfall 1 (same-pitch collision causing stuck notes); Pitfall 9 (arp path missing the reference count).
**Research flag:** No research phase needed. `int[N]` note-count pattern is confirmed JUCE community standard; all API surfaces are existing.

### Phase 4: Sub Octave Per Voice

**Rationale:** Depends on `sentChannel_` (Phase 3) for the channel-snapshot pattern it directly mirrors. DSP is trivial (4 extra note-on/off calls per trigger); the audit of all emission sites including the R3+pad gamepad shortcut is the high-effort part.
**Delivers:** `subOct0..3` APVTS bools; `sentSubOctPitch_[4]`; `looperActiveSubPitch_[4]`; single emission helper covering pad, looper, and arp paths; panic/reset/DAW-stop note-off flush; `rightStickHeld_` atomic in `GamepadInput`; R3+pad combo with `prevR3WithVoice_[4]` edge detection; per-pad UI sub-oct toggle.
**Addresses:** Per-voice sub-octave differentiator; R3+pad ergonomic shortcut.
**Avoids:** Pitfall 2 (orphaned note-off on pitch change mid-hold); Pitfall 12 (missing looper gate path); Pitfall 14 (missing looper reset/DAW-stop flush).
**Research flag:** No research phase needed. Pattern is a direct extension of `looperActivePitch_`; all edge cases are documented in FEATURES.md and PITFALLS.md.

### Phase 5: LFO Recording

**Rationale:** Requires Phase 1 (looper anchor fix) as a hard prerequisite. Self-contained to `LfoEngine` — no note-emission paths touched. Highest new-code complexity of v1.5; building it after Phases 1-4 ensures foundational infrastructure is stable.
**Delivers:** `RecState` enum + atomic in `LfoEngine`; `recBuf_[8192]` ring buffer; `arm()`/`clear()`/`getRecState()` API; `process()` state machine (Unarmed/Armed/Recording/Playback); `maxCycleBeats` wired to live looper length; ARM/CLEAR/blink UI per LFO axis; gray-out of shape/freq/phase/level controls during playback; Distort remains live.
**Addresses:** LFO recording differentiator; Distort-stays-live contract; beat-indexed capture at block rate.
**Avoids:** Pitfall 4 (FIFO overflow — separate ring buffer, not LooperEngine); Pitfall 5 (anchor poison — Phase 1 prerequisite); Pitfall 10 (double-distortion — record pre-distortion `evaluateWaveform()` samples).
**Research flag:** No research phase needed. `ProcessParams::maxCycleBeats` already exists. Capacity math is verified first-principles: 8192 entries at 86 blocks/sec covers 95 seconds — sufficient for the slowest 16-bar loop at 30 BPM.

### Phase 6: Left Joystick X/Y Target Expansion

**Rationale:** Extends existing `filterXMode`/`filterYMode` APVTS choice params by appending new entries. Deferred until after Phases 3-4 to avoid changing the APVTS layout while note-tracking additions are in progress.
**Delivers:** Extended `filterXMode` (7 entries: Cutoff/VCF LFO/Mod Wheel/LFO Shape/LFO Level/LFO Freq/Arp Gate) and `filterYMode` (5 entries: Resonance/LFO Rate/LFO Level/LFO Freq/Arp Gate); `pendingLStickX_/Y_` atomics; step-19 new branches writing pending atomics instead of emitting CC; `timerCallback` reading pending atomics and calling `setValueNotifyingHost`; LFO Shape hysteresis mapping (7 bands, ±0.04 boundary); updated dropdown labels.
**Addresses:** Left-stick modulation target expansion differentiator; LFO freq/shape/level and arp gate len reachable from gamepad without mouse.
**Avoids:** Pitfall 8 (`setValueNotifyingHost` from audio thread — pending-atomic pattern); `AudioParameterChoice` normalization breakage (append-only, existing indices 0/1/2 and 0/1 preserved).
**Research flag:** One implementation-time verification: inspect JUCE 8 `AudioParameterChoice.cpp` to confirm index is stored as raw integer (not normalized float). If normalized, add a value migration shim for existing user presets before extending the choice list.

### Phase 7: Gamepad Option Mode 1 Remapping + R3 Removal

**Rationale:** Last feature phase because it depends on sub-octave params and `rightStickHeld_` (Phase 4) for the R3+pad gesture. Pure processBlock dispatch change — zero new DSP.
**Delivers:** Mode-1 face-button dispatch (Circle=arp on/off, Triangle=arp rate step, Square=arp order step, Cross=RND Sync toggle); R3 standalone panic line deletion; R3+pad sub-oct toggle using `prevR3WithVoice_[4]` (already added in Phase 4); `pendingRndSyncToggle_` pending atomic; face-button looper dispatch gated to mode 0.
**Addresses:** Gamepad Option Mode 1 differentiator; arp control during live performance without touching the mouse.
**Avoids:** Pitfall 6 (Circle fires `looperDelete_` unconditionally — mode guard); Pitfall 7 (R3+pad BT timing miss — `rightStickHeld_` continuous state + hold window); Pitfall 11 (RND Sync `setValueNotifyingHost` from audio thread — pending-atomic pattern); Pitfall 13 (arp step reset on Circle — document as expected behavior, not a bug).
**Research flag:** No research phase needed. Pure dispatch logic extension of the existing `optionMode` D-pad pattern.

### Phase 8: Integration Testing + Installer Update

**Rationale:** After all features are in place, validate interactions that span subsystems and cannot be tested in isolation.
**Delivers:** End-to-end testing of all feature combinations (single-channel + sub-oct + arp simultaneously; LFO recording while looper is cycling; Option Mode 1 with BT reconnect mid-session); updated installer; v1.5 tag.
**Addresses:** All remaining cross-feature edge cases from FEATURES.md.
**Research flag:** No research needed — standard integration testing.

### Phase Ordering Rationale

- Phase 1 before Phase 5: Pitfall 5 is a hard dependency — anchor bug corrupts LFO recording seams.
- Phase 2 is independent: BT fix isolated to `GamepadInput.cpp`; placed second to unblock gamepad testing.
- Phase 3 before Phase 4: `sentChannel_` and `noteCount_` infrastructure established in Phase 3 is directly reused by sub-octave in Phase 4.
- Phase 4 before Phase 7: R3+pad combo requires `subOct0..3` params and `rightStickHeld_` from Phase 4.
- Phase 6 after Phases 3-4: APVTS layout changes are consolidated after note-tracking additions stabilize.
- Phase 5 after Phase 1: mandatory; LFO recording correctness depends on anchor fix.
- Phase 8 after Phases 1-7: integration testing requires all features present.

### Research Flags

Phases with standard patterns (no research phase needed):
- **Phase 1 (Looper anchor fix):** Isolated logic defect; confirmed structural diagnosis.
- **Phase 2 (BT reconnect):** SDL2 API surfaces confirmed in 2.30.9 wiki; fix pattern confirmed from raylib PR #4724 reference implementation.
- **Phase 3 (Single-channel):** `int[N]` note-count pattern is JUCE community standard; no new API surfaces.
- **Phase 4 (Sub octave):** Direct extension of `looperActivePitch_` pattern; fully documented in FEATURES.md and PITFALLS.md.
- **Phase 5 (LFO recording):** `ProcessParams::maxCycleBeats` already exists; ring buffer is plain `std::array`; capacity math verified.
- **Phase 7 (Option Mode 1):** Pure dispatch logic; no new API surfaces.

Needs implementation-time verification (not a full research phase):
- **Phase 6 (Left-stick targets):** Verify `AudioParameterChoice` index storage in JUCE 8 source before extending choice lists. If normalized-float storage, add a value migration path for existing presets.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All API claims verified against JUCE 8 official docs, SDL2 2.30.9 wiki, and direct source inspection. Zero new dependencies confirmed. |
| Features | HIGH | Table stakes derived from musical semantics and direct codebase inspection. Edge cases documented from first principles. Two bug-fix root causes rated MEDIUM (inferred from issue trackers and code inspection, not debugger-confirmed with stack traces). |
| Architecture | HIGH | All integration points verified by reading full `processBlock` (1300+ lines) and all header files. Dependency graph is acyclic. Build order confirmed safe by direct inspection. |
| Pitfalls | HIGH | All 14 pitfalls derived from direct v1.4 source analysis with specific line numbers cited. No speculative or generic pitfalls included. |

**Overall confidence:** HIGH

### Gaps to Address

- **SDL2 BT crash exact root cause (MEDIUM):** Two-failure-mode analysis (double-open during BT re-enumeration; wrong-device close via instance-ID mismatch) is inferred from SDL2 issue tracker. The deferred-open + `GetAttached()` fix is safe regardless of root cause. If crash persists post-fix, capture `SDL_SetHint("SDL_LOGGING", "all")` event sequence in a debug build.

- **Looper anchor exact scenario (MEDIUM):** Either (a) `loopStartPpq_` set at record stop instead of start, or (b) off-by-one at bar boundary. Add `DBG()` to `anchorToBar()` before writing the fix to confirm which scenario applies.

- **`AudioParameterChoice` normalization in JUCE 8 (MEDIUM):** Inspect JUCE 8 `AudioParameterChoice.cpp` at Phase 6 implementation time to confirm append-only choice extension preserves existing user preset values. If normalized-float storage is confirmed, add a value migration shim.

- **LFO recording Distort contract (low priority):** Clarify the exact playback formula — `liveDistortion * noise + bufferedSample` or a different blend — before implementing Phase 5 to avoid a second refactor. Document the intended behavior in the phase plan.

---

## Sources

### Primary (HIGH confidence)
- [JUCE 8 AudioPlayHead::PositionInfo API](https://docs.juce.com/master/classAudioPlayHead_1_1PositionInfo.html) — `getPpqPosition`, `getBpm`, `getIsPlaying` confirmed; all three already in use by LfoEngine and LooperEngine
- [JUCE AbstractFifo Class Reference](http://docs.juce.com/master/classAbstractFifo.html) — SPSC design rationale; reason LFO recording uses plain `std::array` instead
- [SDL2 SDL_ControllerDeviceEvent wiki](https://wiki.libsdl.org/SDL2/SDL_ControllerDeviceEvent) — `which` is joystick instance ID on REMOVED events, device index on ADDED events
- [SDL2 SDL_GameControllerOpen wiki](https://wiki.libsdl.org/SDL2/SDL_GameControllerOpen) — device index at open does not equal future instance ID
- Direct source inspection: `Source/PluginProcessor.cpp` (full processBlock, 1300+ lines), `Source/PluginProcessor.h`, `Source/LfoEngine.h`, `Source/LooperEngine.h`, `Source/TriggerSystem.h`, `Source/GamepadInput.h`, `Source/GamepadInput.cpp`, `.planning/PROJECT.md`

### Secondary (MEDIUM confidence)
- [SDL2 issue #3697](https://github.com/libsdl-org/SDL/issues/3697) — PS4 USB crash on reconnect; crash on rapid re-pair confirmed
- [SDL2 issue #3468](https://github.com/libsdl-org/SDL/issues/3468) — instance ID vs device index confusion on reconnect events
- [Raylib PR #4724](https://github.com/raysan5/raylib/pull/4724) — reference implementation of instance-ID guard for BT reconnect fix
- [JUCE forum: stuck notes in MidiKeyboardState](https://forum.juce.com/t/possible-stuck-notes-in-midikeyboardstate/10316) — note-array tracking is established JUCE plugin pattern
- [JUCE forum: MIDI events lost causing stuck notes](https://forum.juce.com/t/midi-events-lost-causing-stucked-notes/65518) — confirms custom ref-count approach
- KVR Audio DSP forum — MIDI same-pitch collision: reference count is community consensus
- MIDI spec (midi.teragonaudio.com) — duplicate note-on behavior is undefined in the MIDI specification
- [JUCE ArpeggiatorPluginDemo source](https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/ArpeggiatorPluginDemo.h) — sample-counter timing identified as drift-prone; ppqPosition approach recommended instead

### Tertiary (LOW confidence)
- LFO distortion behavior: Ableton M4L LFO "Jitter" documented behavior used as reference for additive-noise distortion model — validate against existing `LfoEngine` LCG implementation before finalizing the playback formula for Phase 5.

---
*Research completed: 2026-02-28*
*Ready for roadmap: yes*
