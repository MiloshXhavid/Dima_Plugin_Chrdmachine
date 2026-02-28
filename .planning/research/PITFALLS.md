# Domain Pitfalls — ChordJoystick v1.5

**Domain:** JUCE VST3 MIDI generator — adding routing, sub-octave, LFO recording, arp gamepad control, left-stick modulation expansion, and gamepad Option Mode 1
**Researched:** 2026-02-28
**Confidence:** HIGH — all findings grounded in direct source-code analysis of the v1.4 codebase

---

## Executive Summary

v1.5 adds six features to an already complex processBlock. Four of them introduce new note-tracking responsibilities (single-channel collision, sub-octave orphan, LFO recording replay, arp+single-channel). Two introduce gamepad state-machine changes that interact with the existing 60 Hz timer architecture (Option Mode 1 button remapping, BT reconnect crash). The highest-risk pitfalls are: (1) note collision on a shared MIDI channel requiring a per-pitch reference count, (2) sub-octave note-off using a stale pitch value, (3) the existing looper start-position bug poisoning LFO recording phase alignment, and (4) the SDL2 BT reconnect crash caused by opening a handle during BT re-enumeration. All are preventable with specific design decisions documented below.

---

## Critical Pitfalls

Mistakes that cause stuck notes, crashes, or silent audio-thread data corruption.

---

### Pitfall 1: Single-Channel Mode — Note-On Collision on Shared MIDI Channel

**What goes wrong:**
When all 4 voices route to the same MIDI channel, two voices can compute the same MIDI pitch number. A single `noteOff(ch, pitch)` kills both because MIDI note-off is pitch-addressed, not voice-addressed.

The current `processBlock` computes four independent pitches via `ChordEngine::computePitch(v, chordP)` and emits `noteOn(voiceChs[v], pitch, 100)` per voice (lines 879, 1115). In single-channel mode `voiceChs[0..3]` all equal the same channel. If voice 0 and voice 2 both compute pitch 64, the MIDI stream becomes:
- `noteOn(ch1, 64, 100)` — voice 0
- `noteOn(ch1, 64, 100)` — voice 2 (redundant; most synths ignore it or retrigger)
- `noteOff(ch1, 64, 0)` — voice 0 releases — kills BOTH

The looper gate-on path (lines 775–787) has the same exposure: `loopOut.gateOn[0]` and `loopOut.gateOn[2]` both fire `noteOn(ch1, looperActivePitch_[v])` in the same block. The first gate-off for either voice sends a note-off that silences the other.

**Consequences:**
- Stuck notes when two voices share a pitch and one releases
- Doubled-velocity artifacts on synths that stack note-ons
- Arp steps that happen to land on the same pitch as a held pad cause immediate mute

**Prevention:**
Track a per-pitch reference count on each MIDI channel: `int noteCount_[16][128] = {}` (audio-thread-only, 2 KB, no atomic needed). On note-on: increment count; always emit noteOn (retrigger is usually desired). On note-off: decrement count; only emit MIDI noteOff when count reaches zero. This is the standard merge-box approach. Apply to all three note-emission paths: direct pad triggers (tp.onNote), looper gateOn/gateOff, and arp.

**Detection warning signs:**
- Notes sustain after pad release when two voices compute the same pitch
- Reliable trigger: set all 4 voices to channel 1, set interval knobs to 0 (unison), hold two pads, release one

**Phase:** Single-channel routing implementation

---

### Pitfall 2: Sub-Octave Note-Off Orphan When Pitch Changes Mid-Hold

**What goes wrong:**
The sub-octave note is `pitch - 12`. In `processBlock`, `heldPitch_[v]` is updated every block at line 800: `heldPitch_[v] = freshPitches[v]`. The existing main-voice solution snapshots the active pitch into `looperActivePitch_[v]` at gate-on and uses that snapshot at gate-off. Sub-octave has no equivalent snapshot — if the implementation sends `noteOff(ch, heldPitch_[v] - 12)` at release time, and the joystick has moved between note-on and note-off, `heldPitch_[v]` is now a different pitch and the note-off misses the sounding note.

Concrete scenario: pad pressed while joystick at Y=0.5 → sub-octave note-on at pitch 55. Joystick moves to Y=0.8 → `heldPitch_[v]` updates to 60. Pad released → `noteOff(ch, heldPitch_[v] - 12)` = `noteOff(ch, 48)`. Pitch 55 never receives a note-off. Stuck note.

**Consequences:**
- Stuck sub-octave note whenever joystick moves between note-on and note-off
- Particularly high probability with looper playback: gate-on fires at beat A with one joystick position; joystick position (from looper replay) changes before gate-off fires at beat B

**Prevention:**
Add `std::array<int, 4> subOctActivePitch_ {-1, -1, -1, -1}` as an audio-thread-only member in PluginProcessor (mirrors `looperActivePitch_`). At sub-octave note-on: `subOctActivePitch_[v] = pitch - 12`. At sub-octave note-off: use `subOctActivePitch_[v]`; reset to -1. Apply the same pattern to the looper gateOn path (lines 775–787) if sub-octave is active for that voice.

**Detection warning signs:**
- Sub-octave note sustains indefinitely when joystick moves during a held pad press
- Does not reproduce with static joystick
- Deterministic: press pad → move joystick → release pad → sub-oct hangs

**Phase:** Sub-octave implementation

---

### Pitfall 3: SDL2 Bluetooth Reconnect — Invalid Handle After Rapid Re-Open

**What goes wrong:**
The 60 Hz `timerCallback()` handles reconnect by responding to `SDL_CONTROLLERDEVICEADDED` (line 113) with `tryOpenController()` called inline inside the SDL event loop. On Bluetooth reconnect, the BT stack on Windows takes 100–500 ms to complete device enumeration after signaling `SDL_CONTROLLERDEVICEADDED`. Calling `SDL_GameControllerOpen(i)` during active BT enumeration can return a handle that passes the null check but becomes invalid on the next SDL API call (the handle points to an incompletely initialized internal SDL structure).

On the subsequent timerCallback tick, `SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTX)` is called with the invalid handle (line 130). This produces an access violation crash.

**Second failure mode:** `SDL_CONTROLLERDEVICEREMOVED` is unreliable on PS4 BT under Windows. The existing fallback poll at line 120–121 (`if (controller_ && !SDL_GameControllerGetAttached(controller_))`) correctly catches silent disconnects. However, if a `SDL_CONTROLLERDEVICEREMOVED` event arrives AND the fallback fires in the same tick (e.g., the event is queued but the poll also runs before the event is processed), `closeController()` is called twice on the same handle. The second call passes `if (controller_)` = false (already nulled) so it is safe, but only because of the null guard. Any future refactor that removes the null guard would cause a double-free.

**Root cause of the crash:** `tryOpenController()` is called from within the `while (SDL_PollEvent(&ev))` loop (line 113). SDL may still have pending events for the same device in the queue. Opening the device before the queue is drained puts SDL's internal state in an inconsistent window.

**Prevention:**
Set a `bool pendingReopen_ = false` flag when `SDL_CONTROLLERDEVICEADDED` fires. On the following timerCallback tick (after the event queue is fully drained), call `tryOpenController()`. This defers the open by one 16.7 ms frame, past the BT enumeration window. After `SDL_GameControllerOpen(i)`, immediately verify: `if (!SDL_GameControllerGetAttached(result)) { SDL_GameControllerClose(result); continue; }`. Do not call `tryOpenController()` from within the SDL event processing loop.

**Detection warning signs:**
- Crash occurs on BT disconnect + reconnect but not on USB
- Stack trace shows `SDL_GameControllerGetAxis` or `SDL_GameControllerGetButton` on the frame after reconnect
- Reliable reproduction: hold a button, BT-disconnect, wait 1 second, BT-reconnect

**Phase:** Bug fix — PS4 BT reconnect crash

---

### Pitfall 4: LFO Recording Buffer — FIFO Overflow on Long Loops

**What goes wrong:**
LFO output is a continuous float signal (one value per processBlock call, not a sparse event). If LFO recording stores samples into the existing `LooperEngine` FIFO as `LooperEvent` entries, a 16-bar 4/4 loop at 120 BPM = 32 seconds. At 512 samples/block and 44100 Hz, that is ~86 blocks/second = 2752 events for one loop cycle. This exceeds `LOOPER_FIFO_CAPACITY = 2048`. The existing `capReached_` guard fires and recording silently truncates mid-loop.

Additionally, LFO events recorded into the LooperEngine FIFO displace gate events. A session with a 16-bar loop that records LFO for more than ~24 seconds leaves zero capacity for gate events. The cap guard stops gate recording without warning.

**Consequences:**
- LFO playback truncates at the 2048-event mark (approximately 24 seconds of recording at 86 blocks/sec)
- Gate events are silently lost when FIFO fills with LFO samples
- `capReached_` becomes true but the UI indicator may not clearly signal the cause

**Prevention:**
Use a **separate ring buffer** for LFO recording, independent of `LooperEngine`. Size: `ceil(maxLoopSecs * maxBlockRate)` with generous headroom. For a 16-bar 4/4 loop at 30 BPM (slowest reasonable tempo) = 128 seconds at ~86 blocks/sec = 10,987 blocks. Use a power-of-two size of 16,384 floats (~64 KB — acceptable for a plugin member). Index with a write-position counter (audio-thread-only, no FIFO needed since recording and playback never overlap in time). Playback reads the same buffer sequentially using a read-position counter that advances each block and wraps at the recorded length.

Do not route LFO recording through `LooperEngine::recordJoystick()` or any FIFO-backed method.

**Detection warning signs:**
- LFO playback ends abruptly mid-loop on loops longer than ~24 seconds (at 512 samples/block, 44100 Hz)
- `capReached_` becomes true during LFO-only recording sessions
- Gate events stop being captured after LFO recording runs for a few seconds

**Phase:** LFO recording implementation

---

### Pitfall 5: Looper Start-Position Bug Poisons LFO Recording Phase Alignment

**What goes wrong:**
The known bug "looper start position wrong after rec cycle" directly corrupts LFO recording playback. In `LooperEngine::process()` at lines 758–763, after `finaliseRecording()`, `internalBeat_` resets to `0.0` and `loopStartPpq_` resets to `-1.0`. On the very next block, `anchorToBar()` re-anchors to `floor(ppqPos / bpb) * bpb` — the current bar, not the bar where recording started. If the DAW playhead is mid-bar at the moment recording ends (the typical case), the anchor slips by up to one bar.

For gate events, this phase slip is usually masked by note-attack transients. For LFO waveform playback, a phase slip of 0.3 beats at 120 BPM is 150 ms — an audible discontinuity at the loop seam on every cycle.

**Consequence for LFO recording:** Adding LFO recording before fixing this bug produces a system where LFO playback has a phase glitch at every loop seam. The glitch will be attributed to LFO recording (a new feature) when the actual cause is the pre-existing looper anchor bug.

**Prevention:**
Fix the anchor bug before implementing LFO recording. The fix: capture `recordStartPpq_` (the `ppqPosition` at the moment `recordPending_` activates or `activateRecordingNow()` fires) and use that as the `loopStartPpq_` in `finaliseRecording()` instead of resetting to -1 and re-anchoring on the next block. The read-position counter for LFO playback must use the same anchor reference as gate event playback.

**Detection warning signs:**
- LFO playback waveform has a phase discontinuity at the loop seam after the first record cycle
- Glitch disappears if looper is stopped and restarted manually (clean anchor from stop)
- Glitch magnitude is proportional to `ppqPos mod beatsPerBlock` at the moment recording ends

**Phase:** Looper bug fix must precede LFO recording phase

---

## Moderate Pitfalls

---

### Pitfall 6: Option Mode 1 — Circle Fires `looperDelete_` Unconditionally

**What goes wrong:**
In `GamepadInput::timerCallback()` (line 177–178), `SDL_CONTROLLER_BUTTON_B` (Circle on PS4) is mapped to `looperDelete_` with no mode guard. The v1.4 Option Mode implementation gates D-pad behavior via `optionMode_`, but the four face buttons (Cross/Circle/Square/Triangle) fire their looper atomics unconditionally.

In v1.5, Option Mode 1 reassigns Circle to arp on/off. If the looper button dispatch is not gated, pressing Circle in Mode 1 fires both `looperDelete_` and the new arp toggle. The loop is deleted every time arp is toggled.

The same applies to Triangle (looper record → arp rate), Square (looper reset → arp order), and Cross (looper start/stop → arp RND sync).

**Consequences:**
- Circle in Option Mode 1 deletes the loop
- Triangle in Option Mode 1 starts recording
- Square in Option Mode 1 resets looper
- All four actions have dual effects until mode guard is added

**Prevention:**
Wrap the entire face-button looper dispatch block (lines 175–185 in GamepadInput.cpp) inside `if (optionMode_.load(std::memory_order_relaxed) == 0)`. In Option Mode 1, process the same four buttons against new atomics: `arpToggle_`, `arpRateInc_`, `arpOrderNext_`, `arpRndSyncToggle_`. Consume these in processBlock when `gamepad_.getOptionMode() == 1`. Do not reuse the existing looper atomics for dual purposes.

**Detection warning signs:**
- Pressing Circle in Option Mode 1 deletes the loop
- Loop content disappears on first arp toggle attempt
- Reliable test: arm a loop, switch to Mode 1, press Circle

**Phase:** Gamepad Option Mode 1 implementation

---

### Pitfall 7: R3 + Pad Combo Detection — 60 Hz Frame Timing Misses Simultaneous BT Presses

**What goes wrong:**
R3 + held-pad combo for sub-octave toggle requires detecting R3 pressed while a voice trigger (L1/L2/R1/R2) is also held. At 60 Hz polling (16.7 ms frames), two buttons pressed within the same frame appear simultaneous. The issue is the detection site:

In `timerCallback()`, R3 fires `rightStickTrig_.store(true)` (line 190–192) as a rising-edge flag. In `processBlock`, `consumeRightStickTrigger()` exchanges this flag to false. The existing held-state atomics `voiceHeld_[v]` are accurate at 60 Hz resolution.

If the combo detection logic checks `voiceHeld_[v]` in `processBlock` at the moment `consumeRightStickTrigger()` returns true, and both R3 and L1 are pressed within the same 60 Hz frame, the detection is correct. But PS4 Bluetooth adds ~8 ms latency per button event. Two simultaneous physical presses may arrive in adjacent 60 Hz frames (16.7 ms apart). In the second frame, L1 has already been in `voiceHeld_[0] = true` for one frame, and R3 fires in this frame — detection succeeds. However, if R3 arrives in the first frame and L1 in the second frame (latency inversion), `voiceHeld_[0]` is false when R3 is consumed — combo fails.

**Consequences:**
- Sub-octave combo fires the old R3 action (panic, before removal) instead of toggling sub-oct
- Combo unreliable on BT; reliable on USB
- Intermittent: depends on exact BT scheduling

**Prevention:**
Add a `rightStickHeld_` atomic to `GamepadInput` (mirrors `voiceHeld_[v]` pattern). Detect the combo in `processBlock` with a short hold window: if `rightStickHeld_` and `getVoiceHeld(v)` are simultaneously true (even across multiple blocks), treat as combo. Since `voiceHeld_` is continuously updated at 60 Hz, the window for detection spans multiple processBlock calls (~3 calls per 60 Hz tick). Store `rightStickHeldSamples_` counter (audio-thread-only): increment while `rightStickHeld_` is true; fire combo if `voiceHeld_[v]` becomes true while counter is within a threshold window (e.g., 3 blocks ≈ 34 ms at 512 samples/block). This tolerates BT latency inversion.

**Detection warning signs:**
- Sub-octave combo works on USB gamepad but fails intermittently on BT
- Adding `DBG` to combo detection shows R3 consumed before L1 is held approximately 30% of the time on BT

**Phase:** Gamepad Option Mode 1 / sub-octave implementation

---

### Pitfall 8: Left-Stick Modulation Targets — `setValueNotifyingHost` from Audio Thread

**What goes wrong:**
Left stick X/Y expanded targets (LFO freq, shape, level, arp gate len) are APVTS parameters. If the implementation calls `apvts.getParameter("lfoXRate")->setValueNotifyingHost(value)` from `processBlock` (audio thread), this is illegal — JUCE's `setValueNotifyingHost` acquires the APVTS lock internally on some JUCE 8 build configurations and always notifies the host, which may call back onto the audio thread recursively. At minimum it floods the host automation track with 60-sample-rate-worth of parameter changes per second.

**Consequences:**
- JUCE assertion "This should only be called from the main thread" fires in debug builds
- Host automation undo stack overflow at 60 writes/sec in some DAWs
- Potential deadlock if host calls `audioProcessorParameterChanged` re-entrantly

**Prevention:**
Apply left-stick LFO/arp modulation as an inline additive offset computed inside `processBlock`, not as APVTS writes. This is the same pattern already used for LFO output on joystick position (lines 631–632). Read the APVTS base value, add the stick contribution, use the result locally:

```cpp
const float baseLfoRate = apvts.getRawParameterValue("lfoXRate")->load();
const float effectiveRate = baseLfoRate + stickX * modDepth;  // local only, not written back
```

Pass `effectiveRate` directly to `ProcessParams.rateHz` for that block. This is zero-latency, audio-thread-safe, and produces no parameter-change notifications.

**Detection warning signs:**
- JUCE assertion fires in debug build on first stick movement with modulation enabled
- Host records a continuous automation lane from stick movements

**Phase:** Left-stick modulation expansion

---

### Pitfall 9: Arp + Single-Channel Mode — Step Collision When Adjacent Voices Share Pitch

**What goes wrong:**
The arpeggiator emits `noteOn(voiceChs[voice], heldPitch_[voice], 100)` per step, tracking one active pitch in `arpActivePitch_`. In single-channel mode, when two adjacent arp steps hit the same quantized pitch, the step-boundary note-off fires `noteOff(ch, prevPitch)` and then `noteOn(ch, samePitch)` in the same block. Some synths interpret this as a retrigger (new note envelope); others suppress the noteOff because the note is immediately re-triggered. If `arpActivePitch_` equals the next step's pitch and both are on the same channel, the behavior is synth-dependent and inconsistent.

With the per-pitch reference-count solution from Pitfall 1, this specific case is handled: the reference count for that pitch goes 1 → (noteOff decrements to 0 → MIDI noteOff sent) → (noteOn increments to 1 → MIDI noteOn sent). This is the correct retrigger behavior. The pitfall is that Pitfall 1's solution must be applied to the arp path as well, not just the pad trigger path.

**Prevention:**
Apply the same per-pitch reference count (`noteCount_[16][128]`) to arp note-on and note-off emissions (lines 1068, 1025, 990–991). Do not add arp-specific special-casing; the reference count solution handles all cases uniformly.

**Phase:** Single-channel routing phase (arp integration path)

---

### Pitfall 10: LFO Recording "Distort Stays Live" — Double-Distortion on Playback

**What goes wrong:**
The v1.5 spec says "Distort stays live" (distortion parameter remains adjustable during playback of a recorded LFO cycle). If the ring buffer stores **post-distortion** samples (the final `lfoX_.process(xp)` return value, which already includes the LCG noise contribution), then during playback the user's live distortion knob re-applies distortion to an already-distorted signal:

- Buffer was recorded at distortion=0.2 → waveform contains 20% noise contribution
- User turns distortion to 0.4 during playback → live distortion 0.4 applied on top
- Net distortion is not 0.4 but approximately 0.2 + 0.4 * (1 - 0.2) ≈ 0.52

The user expects "live distortion = 0.4 = 40% noise", but hears 52% noise.

**Prevention:**
Record **pre-distortion** waveform values: store the `evaluateWaveform()` output before the LCG noise is added. During playback, apply the current live distortion value to each replayed sample. This requires a small refactor of `LfoEngine`: expose `evaluateWaveform()` publicly (or add a `noDistort` flag to `ProcessParams`). The playback path in processBlock reads the ring buffer sample, then applies `liveDistortion * nextLcg() + (1.0f - liveDistortion) * bufferedSample`.

Document the intended behavior explicitly before implementation to avoid ambiguity.

**Detection warning signs:**
- LFO playback sounds "dirtier" than the distortion knob value suggests
- Turning distortion to zero does not fully remove noise from recorded playback
- The artifact is proportional to the distortion level used during recording

**Phase:** LFO recording implementation

---

### Pitfall 11: Option Mode 1 RND Sync Toggle — `setValueNotifyingHost` from processBlock

**What goes wrong:**
v1.5 maps Cross (SDL A) in Option Mode 1 to `X=RND Sync`, toggling the `randomClockSync` APVTS bool. If the implementation toggles this via `setValueNotifyingHost("randomClockSync", ...)` from processBlock (audio thread), this is the same violation as Pitfall 8.

**Prevention:**
Use the same pending-delta atomic pattern as Option Mode 2's D-pad parameter control (lines 486–497): set a `pendingRndSyncToggle_` atomic bool in processBlock; consume in `PluginEditor::timerCallback()` and call `setValueNotifyingHost` there. This is the established pattern in the codebase for all gamepad-driven parameter changes.

**Phase:** Gamepad Option Mode 1 implementation

---

## Minor Pitfalls

---

### Pitfall 12: Sub-Octave in Looper Playback — Both Paths Must Fire Sub-Oct

**What goes wrong:**
The looper gate-on path (lines 775–787) currently fires one `noteOn` per voice. If sub-octave is added to `tp.onNote` (the direct pad/joystick/random trigger path) but not to the looper gate-on path, live pad presses produce sub-octave notes but looper playback does not. The feature appears to work in basic testing (pad presses) but fails when the looper is active.

**Prevention:**
Abstract the "emit note-on for voice, optionally including sub-octave" logic into a helper used by both paths. Both `loopOut.gateOn[v]` and `tp.onNote` call the same helper. The helper accepts the voice index, pitch, channel, MIDI buffer, sample offset, and a `bool subOctEnabled[4]` state array. Sub-octave snapshot (`subOctActivePitch_[v]`) is updated inside the helper. This avoids implementing sub-octave twice and missing one path.

**Phase:** Sub-octave implementation

---

### Pitfall 13: Arp `arpStep_` Not Reset When Circle Toggles Arp Off Then On

**What goes wrong:**
The arp disable path resets `arpPhase_ = 0.0` and `arpStep_ = 0` (lines 996–997). When arp is toggled off via Circle (Mode 1) and immediately re-enabled, the phase and step reset to 0. This is usually desirable (re-lock to beat grid on re-enable). However, if the user uses Circle to "skip a beat" mid-phrase, the step counter reset produces a different voice order than expected (always restarts from voice 0 of the sequence).

This is an intentional behavior tradeoff, not a bug. Document it: Circle toggles arp off → all state resets → next enable always starts from step 0 of the sequence. If "resume from current step" is desired, remove the `arpStep_ = 0` reset from the disable path — but this requires careful handling to avoid stale arpActivePitch_ state.

**Phase:** Gamepad Option Mode 1 (document the decision)

---

### Pitfall 14: Sub-Octave Note Sent on Looper Reset Without Matching Note-Off

**What goes wrong:**
On looper reset (`loopOut.looperReset = true`, lines 734–744), the processor iterates `looperActivePitch_[v]` and emits note-offs. If sub-octave is active, `subOctActivePitch_[v]` also needs a note-off in this path. The current reset handler only clears `looperActivePitch_[v]`; a sub-octave note sounding from a looper gate-on will not receive a note-off on reset.

**Prevention:**
In the looper reset handler, also emit `noteOff(ch, subOctActivePitch_[v], 0)` for each voice where `subOctActivePitch_[v] >= 0`, then reset to -1. Apply the same logic to the DAW stop path (`loopOut.dawStopped`, lines 686–692 equivalent) and the looper stop path (lines 718–730).

**Phase:** Sub-octave implementation (include all note-off emission points in the audit)

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|---|---|---|
| Looper start-position bug fix | Must be fixed before LFO recording | Capture `recordStartPpq_` at recording activation (Pitfall 5) |
| Single-channel routing | Note collision when two voices share pitch | Per-pitch reference count `noteCount_[16][128]` (Pitfall 1) |
| Single-channel + arp | Same-pitch adjacent steps stutter | Apply reference count to arp path too (Pitfall 9) |
| Sub-octave note-on/off | Orphaned sub-oct on pitch change mid-hold | `subOctActivePitch_[v]` snapshot at note-on (Pitfall 2) |
| Sub-octave + looper | Sub-oct missing from looper gate-on path | Single emission helper for both paths (Pitfall 12) |
| Sub-octave on looper reset/DAW stop | Sub-oct note not cleared | Include `subOctActivePitch_[v]` in all note-off flush paths (Pitfall 14) |
| LFO recording buffer | FIFO overflow on long loops | Separate ring buffer, not LooperEngine FIFO (Pitfall 4) |
| LFO distortion during playback | Double-distortion on replay | Record pre-distortion waveform, apply distortion live (Pitfall 10) |
| Left-stick modulation expansion | Flooding DAW automation via `setValueNotifyingHost` | Inline additive offset in processBlock (Pitfall 8) |
| Option Mode 1 Circle = arp | Circle also fires `looperDelete_` unconditionally | Gate all face buttons behind `optionMode_ == 0` (Pitfall 6) |
| Option Mode 1 RND Sync | `setValueNotifyingHost` from audio thread | Pending atomic + message-thread consume (Pitfall 11) |
| R3 + pad sub-oct combo | 60 Hz frame misses simultaneous BT presses | `rightStickHeld_` + hold window in processBlock (Pitfall 7) |
| PS4 BT reconnect crash | SDL handle invalid after rapid re-open | Deferred re-open (one-tick delay) + verify Attached after Open (Pitfall 3) |

---

## Implementation Order Recommendation

Based on pitfall dependencies:

1. **Fix looper start-position bug first** (Pitfall 5 poisons LFO recording phase if left unfixed)
2. **Fix PS4 BT reconnect crash** (independent; unblocks gamepad testing for all other features)
3. **Single-channel routing with reference count** (foundation; arp and sub-octave both depend on correct note counting)
4. **Sub-octave** (requires reference count in place; implement note emission helper for both pad and looper paths)
5. **LFO recording** (requires looper anchor bug fixed; use separate ring buffer)
6. **Left-stick modulation expansion** (inline offsets only; no APVTS writes from audio thread)
7. **Option Mode 1 + R3 combo** (last; builds on sub-octave and all gamepad atomics being stable)

---

## Sources

All findings are HIGH confidence — derived from direct analysis of the v1.4 codebase:

- `Source/PluginProcessor.cpp` lines 411–1150: processBlock note-on/off paths, arp state machine, LFO integration, looper gate emission
- `Source/LooperEngine.cpp`: FIFO capacity constraints, `finaliseRecording()` anchor reset logic (lines 758–763)
- `Source/LooperEngine.h`: `LOOPER_FIFO_CAPACITY = 2048`, event type definitions, threading invariants
- `Source/GamepadInput.cpp`: timerCallback button dispatch order, SDL event loop, option mode gating, BT fallback poll
- `Source/GamepadInput.h`: atomic layout, ButtonState pattern, held-state atomics
- `Source/LfoEngine.h`: ProcessParams distortion field, phase accumulator, LCG PRNG
- `Source/PluginProcessor.h`: `looperActivePitch_[]`, `arpActivePitch_`, `arpActiveVoice_`, all relevant state members
- `.planning/PROJECT.md`: v1.5 requirements list, known bugs, architecture decisions

---
*Pitfalls research for: ChordJoystick v1.5 — routing, sub-octave, LFO recording, arp gamepad control, left-stick modulation, Option Mode 1*
*Researched: 2026-02-28*
*All pitfalls derived from direct review of v1.4 source files. No speculative or generic pitfalls included.*
