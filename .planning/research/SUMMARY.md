# Project Research Summary

**Project:** ChordJoystick v1.1
**Domain:** JUCE VST3 MIDI Effect Plugin — incremental polish release on a shipping v1.0 product
**Researched:** 2026-02-24
**Confidence:** HIGH

## Executive Summary

ChordJoystick v1.1 is a four-feature incremental release on a fully-shipped, lock-free JUCE 8 VST3 MIDI generator. All four features — full MIDI panic sweep, trigger/gate quantization, looper playback position bar, and gamepad controller name display — are implementable entirely within the existing JUCE 8.0.4 and SDL2 2.30.9 stack. No new libraries, no new external dependencies, and no architectural changes to the established threading model are required. The existing patterns for atomic flags, APVTS parameter reads, and the single-timer UI repaint loop cover every new requirement.

The recommended approach is to ship each feature in isolation, ordered by blast radius: panic sweep first (isolated processBlock change, zero new state), then quantize infrastructure (new atomics + new APVTS parameter), then the two pure-UI additions (progress bar, gamepad name). This order ensures the highest-risk item — the lock-free quantize path touching LooperEngine's `playbackStore_` — is built after the simpler changes are validated and the new APVTS parameter is already in place.

The primary risks are in the trigger quantization feature and its interaction with LooperEngine's threading invariant. Post-record quantize MUST use the deferred `pendingQuantize_` flag pattern (set on message thread, serviced inside `process()` on audio thread) — writing directly to `playbackStore_[]` from the message thread while the looper is playing is undefined behavior. The secondary risk is CC121: it must not appear in the panic sweep because downstream VST3 instruments (Kontakt, Waves CLA-76) map CC121 to plugin parameters, causing knobs to jump. Both pitfalls have clear, confirmed prevention strategies documented below.

## Key Findings

### Recommended Stack

The stack is fully locked and validated for v1.1. JUCE 8.0.4, SDL2 2.30.9 static, CMake, C++17, MSVC, and Inno Setup are unchanged. Every v1.1 API needed has been verified directly from source headers in the repo's `build/_deps/` directory.

**Core APIs in play for v1.1 (no new libraries):**
- `juce::MidiMessage` static factories (`allSoundOff`, `allNotesOff`, `controllerEvent`, `pitchWheel`) — confirmed in `juce_MidiMessage.cpp`; used for expanded panic sweep on all 16 channels
- `std::round` / `std::fmod` from `<cmath>` — already included in `LooperEngine.cpp`; the entire quantize algorithm is four lines of arithmetic
- `juce::Component::paint()` + `juce::Graphics::fillRect()` — core JUCE, no extra module; used for the custom `LooperPositionBar` component; do NOT use `juce::ProgressBar` (runs its own internal timer)
- `SDL_GameControllerGetType()` (SDL 2.0.12+) + `SDL_GameControllerName()` (SDL 2.0.0+) — both confirmed in `SDL_gamecontroller.h` from SDK 2.30.9; called on message thread only

**One new APVTS parameter required:** `quantizeSubdiv` — a `juce::AudioParameterChoice` with choices {1/4, 1/8, 1/16, 1/32}, default index 1 (1/8 note). Shared by both live quantize and post-record quantize.

### Expected Features

**Must have — P1 (table stakes for v1.1):**
- Full MIDI panic sweep on all 16 channels — current v1.0 panic covers only the 4 voice channels; stuck notes from external gear on any other channel are not cleared. Required sweep: CC64=0, CC120 (`allSoundOff`), CC123 (`allNotesOff`) per channel. Do NOT include CC121.
- Looper playback position bar — users cannot tell where in the loop they are; standard looper UI expectation (Boss RC, Ableton Looper, SooperLooper)
- Post-record QUANTIZE button — most-requested looper feature after recording; Boss, TC Helicon, and MPC all ship this
- Quantize subdivision selector (1/4, 1/8, 1/16, 1/32) — required by quantize feature; shared between live and post-record modes

**Should have — P2 (differentiators and polish):**
- Live record quantization — tightens performances at recording time; shares subdivision parameter and algorithm with post-record; build in the same phase
- Gamepad controller type in status label ("PS4 Connected" vs "Connected") — low effort, removes ambiguity when multiple controllers exist
- Animated mute state visual feedback — prevents "is it broken?" confusion when plugin is muted
- Section visual grouping in UI (panel borders/fills) — reduces cognitive load

**Defer to v2+:**
- Quantize strength knob (partial snap) — binary on/off sufficient; strength adds UI complexity without proportional value
- Separate quantize grid per voice — over-complicating for v1.1; one global subdivision governs all voices
- Chord name display — requires significant analysis engine work; non-traditional voicings do not map cleanly to named chords

### Architecture Approach

All four features fit cleanly into the existing two-thread model. The existing atomic-flag deferred-request pattern (used by `pendingPanic_`, `deleteRequest_`, `resetRequest_`) governs any write to `playbackStore_[]`. The existing single 30 Hz `juce::Timer` in `PluginEditor` drives all UI animation. No new threads, no new mutexes, no structural changes to any existing component boundary are needed.

**Components modified by v1.1:**

1. `PluginProcessor::processBlock()` — panic block extended to 16-channel CC sweep; `quantizeSubdiv` APVTS parameter added in `createParameterLayout()`
2. `LooperEngine` — new `snapToGrid()` static helper + `subdivIndexToBeats()`; `liveQuantize_` and `quantizeSubdivIdx_` atomics; `quantizePlayback()` method (serviced via `pendingQuantize_` flag inside `process()`); modified `recordGate()` for live snap
3. `PluginProcessor.h` — new forwarding methods: `looperGetPlaybackBeat()`, `looperGetLoopLengthBeats()`, `looperQuantize()`, `looperSetLiveQuantize()`, `looperSetQuantizeSubdiv()`
4. `GamepadInput` — `getControllerName()` accessor; `controllerName_` plain `juce::String` (message thread only); name populated in `timerCallback()` after `SDL_GameControllerOpen()` succeeds
5. `PluginEditor` — new `LooperPositionBar` custom component; `timerCallback()` extended for progress bar read and QUANTIZE button enable/disable gating; gamepad name label update

**processBlock step order (unchanged — v1.1 changes are drop-in):**
1–5: existing (deadzone sync, gamepad poll, DAW playhead, looper process, DAW stop detection)
6. MIDI Panic sweep — extended from 4-channel to 16-channel CC sweep [v1.1]
7–8: existing (midiMuted_ gate, looper gate playback)
9. TriggerSystem::processBlock() → `recordGate()` — live quantize snap applied inside `recordGate()` via atomics [v1.1]
10–13: existing (recording activation, joystick/filter recording, filter CC, looper config sync)

### Critical Pitfalls

1. **CRITICAL: Do NOT send CC121 in panic sweep** — CC121 ("Reset All Controllers") is mapped to plugin parameters by downstream VST3 instruments (Kontakt, Waves CLA-76, others). JUCE's VST3 wrapper routes CC121 through `getMidiControllerAssignment()`, causing knobs to jump. Send only: `CC64=0` (sustain off), `CC120` (`allSoundOff`), `CC123` (`allNotesOff`) per channel. Verify with MIDI monitor that zero CC121 events appear in panic output.

2. **CRITICAL: Post-record quantize MUST use `pendingQuantize_` deferred-request pattern** — `playbackStore_[]` is a plain `std::array` with no mutex; the audio thread reads it every block during playback. Setting an atomic flag from the message thread `onClick` handler and servicing it at the top of `LooperEngine::process()` (audio thread) is the only safe path. The QUANTIZE UI button must also be disabled while the looper is playing. Direct write from message thread while playing = undefined behavior data race.

3. **CRITICAL: Apply `std::fmod(quantized, loopLen)` on all quantized beat positions** — rounding a position near the loop end produces a value equal to `loopLen`. Storing `beatPosition == loopLen` causes the event to be silently missed (no playback window covers `beatPosition >= loopLen`) or double-triggered after wrap. `fmod` is the single guard preventing both failure modes.

4. **Gate-on and gate-off must be snapped by the same delta to preserve gate length** — when live quantizing, the snap delta applied to the gate-on event must also be applied to its paired gate-off event. Snapping gate-on only compresses or extends gate length unpredictably.

5. **Single 30 Hz timer — no second timer** — adding a `juce::Timer` for mute animation or any other purpose doubles timer overhead and creates beat-frequency interference with existing repaints. All animation is driven from the single `startTimerHz(30)` in `PluginEditor` via phase counters. Verify: `grep "startTimerHz\|startTimer" Source/PluginEditor.cpp` must return exactly 1 result.

## Implications for Roadmap

Based on combined research, suggested phase structure for v1.1:

### Phase 1: MIDI Panic Full Sweep

**Rationale:** Isolated processBlock change with zero new state and zero new threading concerns. Highest user-safety impact, lowest implementation risk. Ship and validate in DAW before touching LooperEngine.

**Delivers:** Panic that silences stuck notes on all 16 MIDI channels (CC64=0 + CC120 + CC123 per channel), including notes from external gear, other plugin instances, and sustain-pedal-held notes. Also stops the looper and resets filter CCs.

**Addresses features from FEATURES.md:** Full 16-channel panic sweep (P1 table stakes). Also confirm `pendingCcReset_` or explicit filter CC zeros (CC74, CC71) are emitted in the same panic block without duplication.

**Avoids pitfalls:**
- PITFALL 2: No CC121 in sweep. Panic uses `CC64=0` + `allSoundOff(ch)` + `allNotesOff(ch)` only.
- PITFALL 3: `trigger_.resetAllGates()` stays BEFORE the CC sweep loop. Do not reorder.
- PITFALL 1 (buffer overflow): Sweep is 16 ch × 3 events = 48 events (~240 bytes). Add `midi.ensureSize(256)` in `prepareToPlay()`. Do not expand to CC0–127 sweep.

**Verification:** MIDI monitor shows exactly 48 events (16 × 3) per panic invocation; zero CC121. After panic with all pads released, two consecutive `processBlock` calls produce zero note-on events.

---

### Phase 2: Trigger Quantization Infrastructure

**Rationale:** The quantize feature touches LooperEngine internals and requires a new APVTS parameter. Build and validate the audio-thread path (live quantize + post-record quantize) before adding any UI. The APVTS `quantizeSubdiv` parameter must exist before the UI ComboBox can attach to it.

**Delivers:** `snapToGrid()` static helper + `subdivIndexToBeats()` lookup table; `liveQuantize_` and `quantizeSubdivIdx_` atomics on LooperEngine; `quantizePlayback()` method serviced via `pendingQuantize_` flag inside `process()`; `quantizeSubdiv` APVTS parameter declared; forwarding methods on PluginProcessor; Catch2 test for loop-wrap edge case.

**Addresses features from FEATURES.md:** Live record quantize (P2), post-record QUANTIZE button infrastructure (P1), quantize subdivision selector (P1).

**Avoids pitfalls:**
- PITFALL 5 (data race): `pendingQuantize_` flag pattern. Message thread sets flag only; audio thread services it inside `process()`. Never write `playbackStore_[]` from the message thread while playing.
- PITFALL 4 (double-trigger at wrap): `std::fmod(quantized, loopLen)` on every quantized position before storing. Add `jassert(ev.beatPosition >= 0.0 && ev.beatPosition < loopLen)` in debug builds.
- PITFALL 6 (mid-record subdivision change): Capture `liveQuantizeSubdivBeats_` once at `recordPending_ -> recording_` transition; do not re-read APVTS during an active recording pass.
- PITFALL 9 (wrong parameter read): All new `processBlock` reads use `getRawParameterValue("id")->load()` — never `getParameter()->getValue()`.
- PITFALL 10 (sampleOffset off-by-one): Any new `midi.addEvent()` calls use `jlimit(0, blockSize-1, sampleOff)`.

**Recommended build sub-order within this phase:**
1. Add `quantizeSubdiv` APVTS parameter in `createParameterLayout()`
2. Add `snapToGrid()` + `subdivIndexToBeats()` static helpers in `LooperEngine.cpp`
3. Add `liveQuantize_` and `quantizeSubdivIdx_` atomics; modify `recordGate()` for live snap
4. Add `pendingQuantize_` atomic; implement `quantizePlayback()`; service flag in `process()`
5. Add PluginProcessor forwarding methods
6. Write Catch2 tests: `snapToGrid` wrap edge case; live quantize; post-record quantize

**Verification:** Catch2 test — event at `loopLen - 0.001`, quantize to 1-beat grid, stored as `beatPosition == 0.0`, fires exactly once per loop cycle. TSan clean under post-record quantize stress test.

---

### Phase 3: UI — Looper Position Bar + Quantize Controls

**Rationale:** Pure UI additions with zero audio-thread changes. Depends on Phase 2 forwarding methods existing on PluginProcessor. The progress bar reads existing atomics; the QUANTIZE button and subdivision ComboBox wire to Phase 2 infrastructure.

**Delivers:** `LooperPositionBar` custom component in the looper section (6–8px tall horizontal bar, cyan fill during playback, amber fill during recording); QUANTIZE button (disabled while looper is playing or recording); quantize subdivision ComboBox (1/4, 1/8, 1/16, 1/32) with APVTS attachment; live quantize enable toggle.

**Addresses features from FEATURES.md:** Looper position bar (P1), QUANTIZE button UI (P1), subdivision selector UI (P1), live quantize toggle (P2).

**Avoids pitfalls:**
- PITFALL 8 (second timer): All animation driven from existing `startTimerHz(30)`. No new timer objects. `LooperPositionBar` has no `juce::Timer` member; it is repainted from the editor's `timerCallback()`.
- PITFALL 7 (backward jump at wrap): Store `prevBeat_` in PluginEditor; detect wrap when `newBeat < prevBeat - loopLen * 0.5f`; treat as expected forward wrap, not visual backward jump.
- QUANTIZE button must call `setEnabled(!looperIsPlaying() && !looperIsRecording())` each timer tick — the `pendingQuantize_` invariant requires the looper to be stopped.
- Do NOT use `juce::ProgressBar` — runs its own internal timer; has indeterminate-mode animation incompatible with audio-driven position; does not match the PixelLookAndFeel aesthetic.

**Verification:** `grep "startTimerHz\|startTimer" Source/PluginEditor.cpp` returns exactly 1 result. Progress bar sweeps forward continuously; no backward jump at loop end (visual inspection, 1-bar loop at 180 BPM). QUANTIZE button is greyed out while looper plays.

---

### Phase 4: UI Polish — Gamepad Name + Mute Animation

**Rationale:** Lowest-risk, highest-polish additions. Zero audio-thread impact. Both are message-thread-only changes with no new state beyond a cached string and a phase counter.

**Delivers:** Gamepad status label showing controller type ("PS4 Connected", "Xbox One Connected", "Controller Connected" with SDL raw name fallback) using `SDL_GameControllerGetType()` + name string cached in `GamepadInput::timerCallback()`; animated mute button pulses when `midiMuted_` is true (driven from existing timer via `mutePulsePhase_` counter); optional section visual grouping in the editor.

**Addresses features from FEATURES.md:** Gamepad type display (P2), animated mute state (P2), section visual grouping (P2).

**Avoids pitfalls:**
- PITFALL 8 (second timer): Mute pulse animation uses `mutePulsePhase_` counter incremented in existing `timerCallback()`; no new timer.
- `SDL_GameControllerName()` must only be called on the message thread. Cache to `controllerName_` (plain `juce::String`) in `GamepadInput::timerCallback()`; read from `PluginEditor::timerCallback()` on the same thread. No atomics needed — both timers run on the message thread.
- Copy `SDL_GameControllerName()` return value to `juce::String` immediately; do not store the raw `const char*` pointer (points to an internal SDL buffer).

**Verification:** `grep "startTimerHz\|startTimer" Source/PluginEditor.cpp` still returns exactly 1 result. Status label shows "PS4 Connected" with a PS4 controller attached. Mute button visibly pulses at ~1 Hz when muted; stops when unmuted.

---

### Phase Ordering Rationale

- **Phase 1 first:** Zero new state or threading concerns; fixes a real live-performance safety issue; validates the extended panic path in complete isolation before LooperEngine is touched.
- **Phase 2 before Phase 3:** UI cannot wire to quantize controls that do not exist yet. `quantizeSubdiv` APVTS parameter must be declared before a `ComboBox::AttachmentType` can attach to it. PluginProcessor forwarding methods must exist before `PluginEditor` calls them.
- **Phase 3 before Phase 4:** Progress bar and quantize UI complete the P1 feature set. Phase 4 is pure P2 polish — should not block Phase 3.
- **Phases 3 and 4 are independent** and could be parallelized; they share no code paths.

### Research Flags

Phases with well-documented patterns — no `research-phase` needed:

- **Phase 1 (Panic sweep):** Algorithm fully specified; all CC numbers are MIDI spec constants; integration is 10-line replacement of an existing block.
- **Phase 3 (Progress bar):** Custom component pattern fully specified in STACK.md; all APIs confirmed; 30 Hz timer already in place.
- **Phase 4 (Gamepad name):** Single function call at connect time; threading analysis complete; label wiring already exists.

Phase warranting careful validation during implementation:

- **Phase 2 (Quantize infrastructure):** The `pendingQuantize_` deferred-request protocol and the `fmod` wrap guard are non-obvious. Write Catch2 tests for `snapToGrid()` and the loop-wrap edge case BEFORE integrating into `LooperEngine`. Follow the sub-order in Phase 2 above: helpers first, atomics second, `recordGate()` modification third, `quantizePlayback()` fourth. Each step should be independently reviewable.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All APIs verified directly from source headers in `build/_deps/`; no version ambiguity; confirmed no new libraries needed |
| Features | HIGH for MIDI spec items; MEDIUM for UI patterns | CC numbers are MIDI 1.0 standard (immutable); UI patterns triangulated from Boss RC, Ableton Looper, SooperLooper changelog, and MPC docs |
| Architecture | HIGH | Derived from direct source code analysis of `LooperEngine.h`, `PluginProcessor.cpp`, `PluginEditor.cpp`; threading invariants confirmed from inline comments and ASSERT macros |
| Pitfalls | HIGH | All pitfalls derived from actual v1.0 source review + confirmed JUCE bug tracker entries and forum reports (CC121 VST3 issue; MidiBuffer allocation; 60Hz repaint CPU cost) |

**Overall confidence:** HIGH

### Gaps to Address

- **`fmod` guard on gate-off events:** ARCHITECTURE.md specifies that gate-off retains its original position (preserving gate length). Confirm during Phase 2 implementation whether gate-off events can also land outside `[0, loopLen)` via the live quantize path, and whether they need the `fmod` guard or a clamp. If gate-on is snapped by delta D, apply the same delta to gate-off to preserve gate length rather than snapping gate-off independently.

- **Filter CC reset in panic vs. `pendingCcReset_` path:** FEATURES.md specifies panic should zero filter CCs (CC74, CC71, CC12, CC1, CC76 on the filter channel). ARCHITECTURE.md's panic code snippet does not include this. Confirm during Phase 1 whether `pendingCcReset_` is already triggered by the panic path. If so, avoid emitting duplicate CCs in the same block; if not, add explicit filter CC zeros to the panic block.

- **`midi.ensureSize(256)` in `prepareToPlay()`:** Confirm whether `prepareToPlay()` currently calls `ensureSize` before adding it. One-line addition with no risk; must be in Phase 1.

## Sources

### Primary (HIGH confidence — verified from source in repo)
- `build/_deps/juce-src/modules/juce_audio_basics/midi/juce_MidiMessage.cpp` — CC123 (`allNotesOff`), CC120 (`allSoundOff`), CC121 (`allControllersOff`) confirmed; `allControllersOff` excluded from recommended panic sweep
- `build/_deps/juce-src/modules/juce_audio_basics/midi/juce_MidiMessage.h` — `pitchWheel`, `controllerEvent`, `allSoundOff`, `allNotesOff` signatures confirmed
- `build/_deps/sdl2-src/include/SDL_gamecontroller.h` — `SDL_GameControllerType` enum (14 values) and function signatures confirmed from SDL2 2.30.9
- `Source/LooperEngine.h` — `playbackBeat_` atomic<float>, `getPlaybackBeat()`, `getLoopLengthBeats()`, SPSC FIFO threading invariant, `ASSERT_AUDIO_THREAD()` macro
- `Source/PluginProcessor.cpp` — existing `pendingPanic_` exchange pattern (lines 471–477), processBlock step order
- `Source/PluginEditor.cpp` — `startTimerHz(30)` (line 1100), flash counter pattern (lines 1455–1484), gamepad status label

### Secondary (MEDIUM confidence)
- MIDI 1.0 Specification — CC120, CC121, CC123 semantics (standardized, immutable)
- JUCE bug tracker — VST3 wrapper CC121 side-effects on downstream instruments (Waves CLA-76, Kontakt)
- JUCE forum — MidiBuffer 2048-event pre-allocation; 60Hz repaint timer ~40% CPU cost; single-timer pattern recommendation
- SDL2 Wiki — `SDL_GameControllerGetType` availability since SDL 2.0.12; `SDL_GameControllerName` since SDL 2.0.0
- SooperLooper changelog v1.7.9 — loop position indicator as standard looper UI feature
- Cubase, Logic Pro, iConnectivity MIDI Panic docs — 16-channel all-controllers sweep as industry-standard panic sequence

### Tertiary (LOW confidence — context only)
- KVR Audio forums — looper plugin feature expectations
- MPC forum — "Time Correct" quantization terminology and live quantize behavior descriptions

---
*Research completed: 2026-02-24*
*Ready for roadmap: yes*
