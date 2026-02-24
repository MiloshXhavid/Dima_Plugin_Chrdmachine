# Pitfalls Research

**Domain:** ChordJoystick v1.1 — adding MIDI panic sweep, trigger quantization, and UI polish to a shipping JUCE 8 lock-free plugin
**Researched:** 2026-02-24
**Confidence:** HIGH (pitfalls derived from direct source code review of the actual v1.0 implementation + verified JUCE forum sources; no hypothetical scenarios)

---

## Critical Pitfalls

### Pitfall 1: CC Panic Sweep Floods MidiBuffer and Triggers Heap Allocation on the Audio Thread

**What goes wrong:**
A "full CC reset sweep on all 16 channels" means sending at minimum CC120 + CC123 across 16 channels = 32 events. A naive CC0–127 sweep across 16 channels = 2,048 events in a single `processBlock` call. JUCE's `MidiBuffer` grows dynamically — adding events beyond its pre-allocated capacity calls `ensureSize()` internally, which triggers a heap allocation. Heap allocation on the audio thread causes priority inversion and real-time deadline misses (xruns). In VST3, the `MidiBuffer` passed to `processBlock` may not be pre-allocated by the host at all (unlike VST2/AU).

JUCE forum confirmed: the default pre-allocation was raised to 2,048 events only after a developer reported that 16 channels × 8 CCs per block was already insufficient at 128. A CC0–127 sweep is 128 × 16 = 2,048 events — exactly the new limit, no headroom.

**Why it happens:**
"Full CC reset" is interpreted as "send all 128 CCs" without considering that the audio buffer has a finite pre-allocated capacity. The developer writes a nested loop and does not notice the allocation risk until profiling.

**How to avoid:**
1. Keep the panic sweep to the minimum effective set: CC120 (All Sound Off) + CC123 (All Notes Off) + explicit `noteOff` for each of the 4 known-open pitches. That is at most 4 × (noteOff + CC120 + CC123) = 12 events — well within any buffer.
2. Call `midi.ensureSize(256)` in `prepareToPlay()` so the buffer is pre-grown before any audio block. This is a one-time allocation at init, not at runtime.
3. Do NOT iterate CC0–127 on all 16 channels. That is guaranteed allocation and probable buffer stall.
4. If a wider sweep is ever needed (e.g., for a "deep reset" option), use a state machine: `panicChannelIndex_` atomic, incrementing through 1–16 across 16 consecutive blocks (2 channels per block = 6 events per block, no allocation risk).

**Warning signs:**
- Audio thread CPU spikes at the exact moment panic fires (profiler shows `malloc`/`realloc` in the audio call stack)
- DAW reports xruns immediately after panic button press
- `midi.addEvent()` silently returns false (events dropped, some notes never get note-off)

**Phase to address:** MIDI Panic improvement phase.
**Success criterion:** Panic fires at most 12 MIDI events per block on the 4 voice channels. `ensureSize(256)` called in `prepareToPlay`. Verified with a MIDI monitor that shows ≤ 12 events per panic invocation.

---

### Pitfall 2: CC121 (Reset All Controllers) Corrupts Parameters on Downstream VST3 Instruments

**What goes wrong:**
CC121 value 0 is the MIDI "Reset All Controllers" command. Some VST3 instruments (documented in JUCE bug tracker: Waves CLA-76; also affects Kontakt and others) have MIDI CC assignments mapped to CC121, or their VST3 host wrapper calls `getMidiControllerAssignment()` when CC121 arrives and maps it to a plugin parameter — causing knobs to jump to unexpected positions. The JUCE VST3 wrapper had a confirmed bug where routing CC121 through `toEventList()` incorrectly triggered parameter automation on downstream instruments.

**Why it happens:**
Developers look up "MIDI panic" references and find "send CC121 = 0 (Reset All Controllers)" as a standard recommendation. It IS standard MIDI 1.0 — but VST3's MIDI controller assignment mechanism gives hosts and instruments the ability to bind any CC, including CC121, to any parameter. A "reset" message becomes a "set to 0" automation event for any parameter bound to CC121.

**How to avoid:**
1. Do NOT send CC121 in the panic sweep. The existing `PluginProcessor::processBlock` panic path (lines 473–477) uses `juce::MidiMessage::allNotesOff(ch)` which sends CC123, NOT CC121. This is correct — do not change it.
2. If pitch wheel or modulation wheel reset is desired, send them explicitly: `juce::MidiMessage::pitchWheel(ch, 8192)` (8192 = center in JUCE's 0–16383 mapping) and `controllerEvent(ch, 1, 0)` for mod wheel.
3. If sustain pedal reset is desired: `controllerEvent(ch, 64, 0)` explicitly.
4. The v1.1 improvement to panic should add only CC120 (All Sound Off) before the existing CC123 (All Notes Off) — not replace or augment with CC121.

**Warning signs:**
- Downstream synth knobs jump to unexpected positions when panic fires
- Kontakt or Serum reports parameter automation jumps in the DAW timeline
- Pitch wheel or mod wheel snaps to non-center position despite not touching those controls

**Phase to address:** MIDI Panic improvement phase.
**Success criterion:** Panic output buffer contains no CC121. Verify with MIDI monitor or debug log of `midi` buffer contents after panic fires.

---

### Pitfall 3: Note-On / Note-Off Ordering in Panic Leaves Ghost Notes if Gate Reset Happens After MIDI Output

**What goes wrong:**
If the panic block sends `allNotesOff(ch)` but `trigger_.resetAllGates()` is not called until AFTER the MIDI output section, the next `processBlock` iteration can re-trigger notes. Specifically: the mute guard at line 481 (`if (midiMuted_) return;`) currently sits AFTER the panic block. If panic fires but does NOT mute (i.e., the panic button sends note-offs without entering mute state), TriggerSystem's `gateOpen_[v]` is still true, and the next block's `trigger_.processBlock(tp)` fires a new note-on at the same pitch.

The current v1.0 implementation handles this correctly: panic calls `trigger_.resetAllGates()` before the mute return. The risk is in v1.1 if the panic logic is restructured or a "CC sweep loop" is inserted between the `allNotesOff` calls and the `resetAllGates()` call.

**Why it happens:**
Refactoring the panic section to add a CC sweep loop creates a natural insertion point between the allNotesOff and the resetAllGates. A developer inserts the sweep there without recognising the ordering constraint.

**How to avoid:**
1. Keep the invariant: `allNotesOff()` → `trigger_.resetAllGates()` → (CC sweep) → `flashPanic_`. The CC sweep must come LAST.
2. The MIDI buffer ordering does not matter (allNotesOff and any subsequent CCs are all at `sampleOff=0`), but `resetAllGates()` must happen before any path that reads `gateOpen_[v]`.
3. After `resetAllGates()`, also reset `heldPitch_[v]` to -1 or a sentinel value so the looper gate playback path cannot re-trigger with stale pitches in the same block.
4. If the v1.1 panic path adds CC120 (All Sound Off) before the existing allNotesOff, the order becomes: CC120 → allNotesOff → `resetAllGates()` → CC sweep → `flashPanic_`.

**Warning signs:**
- Single note continues sounding after panic fires
- MIDI monitor shows note-off at sample 0 immediately followed by note-on at sample 0 in the same block
- Stuck notes when using the looper with gates armed at high density

**Phase to address:** MIDI Panic improvement phase.
**Success criterion:** After panic fires with all pads released, the next 2 consecutive `processBlock` calls produce zero note-on events. Verify with a debug counter.

---

### Pitfall 4: Live Trigger Quantization Causes Double-Trigger at the Loop Wrap Boundary

**What goes wrong:**
When live quantization snaps a gate event's beat position to the nearest grid point, that position may land exactly at the loop end (`loopLen`) or just before it. The wrap detection in `LooperEngine::process()` is:

```cpp
bool inWindow = wraps
    ? (ev.beatPosition >= beatAtBlockStart || ev.beatPosition < beatAtEnd)
    : (ev.beatPosition >= beatAtBlockStart && ev.beatPosition < beatAtEnd);
```

An event stored at `beatPosition = loopLen - epsilon` (just before the end) fires correctly on the pre-wrap scan of the block that straddles the loop end. But if quantization snaps it to `loopLen`, `std::fmod(loopLen, loopLen) == 0.0`, and the event is now stored at 0.0. The wrap-window condition `ev.beatPosition < beatAtEnd` (where `beatAtEnd` is just past 0.0) fires it again at loop start — double trigger.

Separately: if quantization snaps to exactly `loopLen` without applying `fmod`, the event is stored with `beatPosition >= loopLen`. The linear scan never finds it (all windows are `[0, loopLen)`), so the gate fires silently never — stuck-open note.

**Why it happens:**
The "next grid point" calculation in beat space naturally produces values like `ceil(pos / gridSize) * gridSize`. For a position close to the loop end, this gives `loopLen` exactly. The developer forgets to wrap the result.

**How to avoid:**
1. After computing the quantized position, always apply: `quantized = std::fmod(quantized, loopLen)`. This is the single guard that prevents both double-trigger AND missing-event.
2. When snapping to "next grid point", explicitly check: `if (quantized >= loopLen) quantized -= loopLen;`
3. Add a `jassert(ev.beatPosition >= 0.0 && ev.beatPosition < loopLen)` in debug builds at the top of the `process()` scan loop. Any out-of-range position is a bug caught immediately.
4. Post-record quantize: after modifying all beat positions, sort `playbackStore_[0..playbackCount_]` by `beatPosition` ascending. This also improves playback correctness when multiple events snap to the same position.

**Warning signs:**
- At loop wrap point, a single note sounds twice in quick succession
- Gate events at the first beat of the next loop cycle are doubled
- After post-record quantize, some notes go silent (stored at `beatPosition >= loopLen`)

**Phase to address:** Trigger quantization phase.
**Success criterion:** Catch2 test — record a gate event at `beatPos = loopLen - 0.001`. Apply quantize with grid 1.0 beat. Verify the stored event has `beatPosition == 0.0` (wrapped) and fires exactly once per loop cycle.

---

### Pitfall 5: Post-Record Quantization Mutates playbackStore_ from the Message Thread While the Audio Thread Reads It

**What goes wrong:**
Post-record quantize (the QUANTIZE button) needs to modify `playbackStore_[]` beat positions in-place. If the UI button callback calls a `quantiseLoop()` method directly on the message thread while `playing_` is true, the audio thread is simultaneously scanning `playbackStore_[]` in `process()`. This is a data race — undefined behavior in C++. The existing SPSC invariant in `LooperEngine` states:

> `playbackStore_` is safe only when `playing_ = false AND recording_ = false`.
> (LooperEngine.cpp line 142–145)

**Why it happens:**
The existing deferred-request pattern (for `deleteRequest_` and `resetRequest_`) is the correct approach, but it is non-obvious. A developer implementing the QUANTIZE button naturally puts the quantize logic directly in the `onClick` lambda, which runs on the message thread.

**How to avoid:**
1. Post-record quantize MUST follow the deferred-request pattern: add `std::atomic<bool> pendingQuantize_ { false }` to `LooperEngine`. Set it from the message thread in the button callback.
2. In `LooperEngine::process()`, at the top of the function (before any scan), check and service `pendingQuantize_`:
   ```cpp
   if (pendingQuantize_.exchange(false, std::memory_order_acq_rel))
   {
       // Modify playbackStore_ here — audio thread only, no concurrent reader
       applyQuantizeToPlaybackStore();
   }
   ```
3. The quantize logic executes on the audio thread when the audio thread is already inside `process()` — there is no concurrent reader at that point.
4. Never expose a method that mutates `playbackStore_[]` callable from outside the audio thread when `playing_` is true.
5. Add `ASSERT_AUDIO_THREAD()` at the top of `applyQuantizeToPlaybackStore()` to catch misuse in debug builds.

**Warning signs:**
- ThreadSanitizer (TSan) reports data race on `playbackStore_` array elements
- Intermittent wrong pitches or wrong beat positions during playback after pressing QUANTIZE
- `ASSERT_AUDIO_THREAD()` fires inside a quantize method (debug build)

**Phase to address:** Trigger quantization phase.
**Success criterion:** `pendingQuantize_` is an `atomic<bool>` set exclusively from the message thread. `playbackStore_` is written exclusively from within `process()` (audio thread). TSan clean under stress test.

---

### Pitfall 6: Live Quantize Subdivison Value Changes Mid-Recording Produces Inconsistent Rhythm

**What goes wrong:**
Live quantize reads the "quantize subdivision" APVTS parameter each block via `getRawParameterValue("quantizeSubdiv")->load()`. If the user changes the knob during an active recording, the first half of the loop snaps to 1/8 and the second half snaps to 1/16. On playback, the rhythm is inconsistent — some hits are grid-aligned to one grid, others to a different grid. This is not a crash but a musical correctness bug.

**Why it happens:**
`processBlock` parameter reads are designed to pick up live knob changes. For most parameters (volume, filter cutoff) this is correct. For quantize subdivision, the intent is "commit this grid for the entire recording pass" — but nothing enforces that commitment.

**How to avoid:**
1. Capture the quantize subdivision value into an audio-thread-only member (`liveQuantizeSubdivBeats_`) at the moment recording starts — specifically inside the `if (recordPending_)` block in `process()` where `recording_` is set to true.
2. Use `liveQuantizeSubdivBeats_` for all live quantize calculations within that recording pass. Do NOT read the APVTS parameter again until the next recording start.
3. Post-record quantize reads the APVTS parameter at the moment the QUANTIZE button is pressed — this is correct (it is a one-shot operation, not a continuous read).

**Warning signs:**
- Rhythm sounds inconsistent after twisting the quantize knob during an active recording
- The pattern drifts differently depending on when the knob was moved

**Phase to address:** Trigger quantization phase.
**Success criterion:** The `liveQuantizeSubdivBeats_` member is written once at `recordPending_ → recording_` transition and not updated again until the next recording start.

---

## Moderate Pitfalls

### Pitfall 7: Looper Progress Bar Displays a Visible Backward Jump at Loop Wrap

**What goes wrong:**
`PluginEditor::timerCallback()` reads `proc_.looper_.getPlaybackBeat()` (which reads `playbackBeat_`, an `atomic<float>` written in `process()`) and maps it to a 0.0–1.0 progress fraction. At loop wrap, `playbackBeat_` jumps from `loopLen - epsilon` to near 0.0 in a single audio block (~10ms at 48kHz/512). The timer fires at 30Hz (~33ms intervals). Between two timer ticks, up to 3 audio blocks execute. The timer may sample `playbackBeat_` at 0.05 on one tick and then 0.03 on the next tick (after the wrap), which looks like a backward jump in the progress bar.

**Why it happens:**
The 33ms timer interval is 3× longer than the typical audio block (~11ms). The atomic float stores only the beat at block start — the timer reads it asynchronously and can sample any point in the range. The wrap appears as a discontinuity because the bar draws linearly.

**How to avoid:**
1. Accept the ~33ms lag — it is imperceptible. Do NOT increase timer frequency to compensate (JUCE forum confirmed: 60Hz repaint timer jumps CPU to ~40%).
2. In `paint()` or in `timerCallback()` before setting the progress fraction: detect wrap by checking if `newBeat < prevBeat - (loopLen * 0.5f)`. If so, treat it as a wrap event, not a backward jump. Either reset the display immediately to the new position (sharp snap, but expected at loop end) or interpolate across the wrap (cosmetic, not necessary).
3. The `playbackBeat_` is `atomic<float>` (4 bytes, lock-free). Reading from the message thread is safe — no race, only staleness.
4. Store `prevBeat_` in `PluginEditor` as a member and compare each timer tick to implement the wrap detection.

**Warning signs:**
- Progress bar sweeps right, then jumps sharply to the left at loop end (backward-looking)
- CPU spike when progress bar timer is added at high frequency

**Phase to address:** UI polish phase (progress bar).
**Success criterion:** Progress bar does not visually jump backward at loop end. Verified by visual inspection with a 1-bar loop at 180 BPM.

---

### Pitfall 8: Animated Mute Button Pulsing Uses a Second juce::Timer

**What goes wrong:**
The mute/panic button needs to pulse visually when `midiMuted_` is true. A developer adds a `juce::Timer` member to `PluginEditor` (or to a custom component) for the animation, separate from the existing `startTimerHz(30)` at line 1100 of `PluginEditor.cpp`. JUCE creates one timer thread tick per registered timer. Two timers firing at similar frequencies can produce two `paint()` calls on the same frame (different timer periods create beat-frequency interference). JUCE coalesces repaints within the same message-pump iteration, so this usually does not cause visual artifacts — but it doubles the timer overhead and can cause subtle repaint CPU spikes.

**Why it happens:**
Treating the animated button as an independent self-contained component with its own timer is a natural instinct. The existing single-timer pattern in the editor is not obvious from the component perspective.

**How to avoid:**
1. Do NOT create a second `juce::Timer` for the mute animation. The existing `timerCallback()` at 30Hz is the animation driver for the entire editor.
2. Add a member `int mutePulsePhase_ = 0` to `PluginEditor`. In `timerCallback()`, if `proc_.isMidiMuted()` is true, increment `mutePulsePhase_` (modulo some period, e.g., 30 for a 1-second pulse at 30Hz). Use `mutePulsePhase_` in `panicBtn_.setColour(...)` or equivalent to cycle the button's visual colour.
3. Call `panicBtn_.repaint()` in `timerCallback()` when muted — one additional `repaint()` call per tick, coalesced by JUCE with the existing pad repaints.
4. The existing `panicFlashCounter_` / `resetFlashCounter_` / `deleteFlashCounter_` pattern (lines 1455–1484 of `PluginEditor.cpp`) is the correct model to follow.

**Warning signs:**
- A second call to `startTimerHz()` or `startTimer()` appears anywhere in `PluginEditor.cpp` or its sub-components
- CPU rises by 5–10% when `midiMuted_` is true compared to unmuted state
- Paint profiler shows the editor painted twice per frame during mute

**Phase to address:** UI polish phase.
**Success criterion:** `grep "startTimerHz\|startTimer" Source/PluginEditor.cpp` returns exactly 1 result. Animation driven from existing `timerCallback()`.

---

### Pitfall 9: New APVTS Parameter Read in processBlock Uses ->getValue() Instead of ->load()

**What goes wrong:**
Adding a "quantize subdivision" APVTS parameter requires reading it in `processBlock`. The correct call is `apvts.getRawParameterValue("quantizeSubdiv")->load()`, which is a lock-free atomic float read. The incorrect call is `apvts.getParameter("quantizeSubdiv")->getValue()` or `apvts.getParameter("quantizeSubdiv")->getNormalisedValue()`, which in some JUCE versions routes through the APVTS listener infrastructure and may acquire an internal lock or trigger a notification — both illegal on the audio thread.

**Why it happens:**
`getRawParameterValue()` is less obviously named than `getParameter()`. Autocomplete suggestions both. `getValue()` works in tests and small projects where there is no contention, but silently risks a lock under DAW load.

**How to avoid:**
The codebase already uses the correct pattern uniformly throughout `PluginProcessor.cpp`. For example:
```cpp
apvts.getRawParameterValue(ParamID::joystickThreshold)->load()
apvts.getRawParameterValue("randomSubdiv0")->load()
```
Every new `processBlock` parameter read MUST use `getRawParameterValue("id")->load()`. Add this to the code review checklist for any v1.1 parameter additions.

**Warning signs:**
- JUCE debug assertion: "This method should not be called from the audio thread" (in some JUCE builds)
- Intermittent audio glitches under DAW CPU load (lock contention between audio thread and UI knob movement)

**Phase to address:** Trigger quantization phase (when adding new parameters).
**Success criterion:** `grep "getParameter(" Source/PluginProcessor.cpp` finds zero matches inside `processBlock`.

---

### Pitfall 10: MidiBuffer::addEvent sampleOffset Must Be in [0, blockSize) — Off-by-One at Block End

**What goes wrong:**
`midi.addEvent(msg, sampleOff)` requires `sampleOff` to be in `[0, blockSize)`. If trigger quantization computes a within-block sample offset for where the quantized event should fire, and the event is exactly at the block end, the computed offset equals `blockSize` — off by one. The event is silently dropped or placed at offset 0 of the following block (host-dependent).

**Why it happens:**
"Quantize to the next beat boundary" computes the beat boundary's sample position. If the boundary falls on exactly `blockSize` samples from the block start, the sample offset is `blockSize` — the most natural computation produces this edge case.

**How to avoid:**
Clamp: `sampleOff = juce::jlimit(0, p.blockSize - 1, computedSampleOffset)`. The existing `TriggerSystem::processBlock` implicitly handles this through subdivision grid calculations. Any new quantization code must apply the same clamp. Add it as a one-liner wherever a computed offset is passed to `midi.addEvent()`.

**Phase to address:** Trigger quantization phase.
**Success criterion:** All new `midi.addEvent()` calls in v1.1 have `jlimit(0, blockSize-1, sampleOff)` on the offset argument.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Drive all animation from single 30Hz timer | Simple, no new objects | All UI updates coarse-grained; smooth animation impossible | Acceptable — 30Hz sufficient for this plugin |
| playbackStore_ mutation via request flag | No mutex, real-time safe | Complex protocol; each new op needs its own atomic flag | Acceptable — consistent with existing reset/delete pattern |
| FIFO capacity fixed at 2048 events | Zero allocation after init | Hard recording limit; dense sessions can hit cap | Acceptable — cap indicator already in UI |
| float atomic for playbackBeat_ (4 bytes) | Lock-free on 32-bit builds | 23-bit mantissa = ~0.008 beat resolution at beat 100 | Acceptable — display only, not used for scheduling |
| Capture quantize subdiv once at record start | Consistent rhythm per recording pass | User cannot change grid mid-record | Acceptable — this is the musically correct behavior |

---

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| JUCE allNotesOff() | Adding CC121 to panic path | CC123 only (what allNotesOff() sends); add CC120; no CC121 |
| APVTS in processBlock | Using `getParameter()->getValue()` | Use `getRawParameterValue("id")->load()` exclusively |
| juce::Timer in PluginEditor | Creating per-component timers for animation | Single editor-level timerCallback(), increment animation phase counters there |
| LooperEngine playbackStore_ mutation | Calling from message thread onClick | Set atomic `pendingQuantize_` flag; service in process() on audio thread |
| MidiBuffer sampleOffset | Passing `blockSize` as offset (off-by-one) | Clamp to `jlimit(0, blockSize-1, offset)` before addEvent() |
| Quantize position at loop end | Storing event at beatPosition == loopLen | Apply `std::fmod(quantized, loopLen)` before storing |

---

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| CC sweep across 16 channels × 128 CCs | xrun at panic press; malloc in audio stack | Minimal panic (12 events max); ensureSize(256) in prepareToPlay | Immediately at first panic press |
| 60Hz+ repaint timer for progress bar | ~40% CPU in idle UI; sluggish knobs | Stay at 30Hz; accept 33ms visual lag | Immediately on timer creation |
| Sort playbackStore_ on audio thread during playback | Unpredictable latency spike | Sort only via pendingQuantize_ request, never during scan | At 2048 events (O(N log N) ~100µs) |
| Post-record quantize from message thread while playing | Data race; UB; intermittent wrong notes | pendingQuantize_ consumed in process() | Non-deterministic; caught by TSan |
| Second juce::Timer for mute animation | CPU spike when muted | Merge into existing timerCallback() | Immediately on timer creation |

---

## "Looks Done But Isn't" Checklist

- [ ] **Panic sweep — no CC121:** MIDI monitor shows zero CC121 events during panic. `allNotesOff()` sends CC123, not CC121 — verify.
- [ ] **Panic gate reset order:** `trigger_.resetAllGates()` executes before any CC sweep code in the panic block. Read the code, do not assume.
- [ ] **Panic event count:** Panic produces at most 12 MIDI events per block (4 voices × noteOff + CC120 + CC123). Count with a debug counter.
- [ ] **Quantize wrap:** An event recorded at `loopLen - 0.001` beats, quantized to the nearest 1-beat boundary, is stored as `beatPosition == 0.0` (not `loopLen`). Test with Catch2.
- [ ] **Quantize thread safety:** The QUANTIZE button `onClick` lambda sets only `pendingQuantize_ = true`. No write to `playbackStore_` occurs on the message thread. Verify by reading the onClick implementation.
- [ ] **Quantize subdivision captured once:** `liveQuantizeSubdivBeats_` is set inside the `if (recordPending_)` block in `process()`. It is never read from the APVTS after that point until the next recording start.
- [ ] **Progress bar backward-jump guard:** The progress bar paint code checks for wrap (newBeat < prevBeat - loopLen/2) before computing the fill fraction. Verify visually with a 1-bar loop at fast tempo.
- [ ] **Single timer:** `grep "startTimerHz\|startTimer" Source/PluginEditor.cpp` returns exactly 1 result.
- [ ] **getRawParameterValue pattern:** `grep "getParameter(" Source/PluginProcessor.cpp` returns zero matches inside processBlock.
- [ ] **sampleOffset clamp:** All new `midi.addEvent()` calls in v1.1 code use `jlimit(0, blockSize-1, sampleOff)`.
- [ ] **ensureSize pre-allocation:** `midi.ensureSize(256)` appears in `prepareToPlay()`.

---

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| CC panic flood | LOW | Add ensureSize(256) in prepareToPlay; reduce panic to 12 events; one-block refactor |
| CC121 corrupts downstream synth | LOW | Remove CC121 from panic path; replace with explicit pitchWheel(8192) + CC64=0 |
| Stuck note after panic (wrong order) | LOW | Move `resetAllGates()` before CC sweep in panic block; 2-line reorder |
| Post-record quantize data race | HIGH | Refactor to pendingQuantize_ atomic flag; cannot be patched in-place without risk |
| Double trigger at loop wrap | MEDIUM | Add `std::fmod(quantized, loopLen)` after every quantize computation; + Catch2 test |
| Second timer for animation | LOW | Merge into existing timerCallback; remove new Timer member; < 1 hour |
| Quantize subdiv changes mid-record | LOW | Capture subdiv value once at record start into `liveQuantizeSubdivBeats_`; 5-line addition |

---

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| CC panic buffer overflow | MIDI Panic phase | MIDI monitor: ≤ 12 events per panic invocation; no xrun |
| CC121 corrupts downstream synth | MIDI Panic phase | MIDI monitor: zero CC121 in panic output |
| Note-on/note-off ordering | MIDI Panic phase | After panic, 2 consecutive blocks produce zero note-on (all pads released) |
| Quantize double-trigger at loop wrap | Trigger Quantization phase | Catch2 test: event at loopLen-0.001, quantize to 1-beat grid → stored as 0.0, fires once/cycle |
| Post-record quantize data race | Trigger Quantization phase | TSan clean; QUANTIZE onClick contains no playbackStore_ write |
| Live quantize subdiv changes mid-record | Trigger Quantization phase | Record with mid-record knob change: all gates snap to subdiv at record start |
| Progress bar backward jump | UI Polish phase | Visual: bar sweeps forward continuously; no backward jump at wrap |
| Second timer for animation | UI Polish phase | grep count: startTimerHz appears exactly once in PluginEditor.cpp |
| getRawParameterValue pattern | All phases adding parameters | grep: no `->getValue()` in processBlock |
| sampleOffset off-by-one | Trigger Quantization + Panic phases | jlimit clamp present on every new midi.addEvent() call in v1.1 |
| ensureSize pre-allocation | MIDI Panic phase | prepareToPlay() contains midi.ensureSize(256) |

---

## Sources

### Primary (HIGH confidence — verified from actual codebase)
- Direct review: `Source/LooperEngine.cpp` — finaliseRecording SPSC invariant (lines 139–145), wrap detection logic (lines 481–484), pendingDelete/pendingReset pattern
- Direct review: `Source/PluginProcessor.cpp` — panic path (lines 471–477), CC dedup (lines 686–695), mute guard (line 481), allNotesOff ordering
- Direct review: `Source/PluginEditor.cpp` — timerCallback (line 1443), startTimerHz(30) (line 1100), existing flashCounter pattern (lines 1455–1484)
- Direct review: `Source/PluginProcessor.h` — pendingPanic_ atomic, pendingAllNotesOff_, pendingCcReset_ pattern (lines 156–158)

### Secondary (MEDIUM confidence — verified via JUCE forum)
- [JUCE forum: MidiBuffer::addEvent in processBlock](https://forum.juce.com/t/midibuffer-addevent-in-processblock/5184) — 2048 event pre-allocation confirmed; allocation risk if exceeded
- [JUCE forum: Bug — VST3 Wrapper and MIDI AllNotesOff event](https://forum.juce.com/t/bug-vst3-wrapper-and-midi-allnotesoff-event/32054) — CC121 side effects on downstream VST3 instruments confirmed
- [JUCE forum: MIDI input quantize to next beat or bar](https://forum.juce.com/t/midi-input-quantize-to-next-beat-or-next-bar/49113) — block-boundary constraint on MIDI sampleOffset
- [JUCE forum: Repaint terrible performance](https://forum.juce.com/t/repaint-terrible-performance-why/59808) — 60Hz repaint timer causes ~40% CPU confirmed
- [JUCE forum: Calling repaint in timer callback](https://forum.juce.com/t/calling-repaint-in-timer-callback-in-several-components/5170) — single timer pattern recommendation

### MIDI Standard (HIGH confidence)
- MIDI 1.0 Specification: CC120 = All Sound Off, CC121 = Reset All Controllers, CC123 = All Notes Off
- JUCE source: `juce::MidiMessage::allNotesOff(ch)` emits CC123 value 0, NOT CC121

---
*Pitfalls research for: ChordJoystick v1.1 — MIDI panic, trigger quantization, and UI polish additions to JUCE 8 lock-free plugin*
*Researched: 2026-02-24*
*All pitfalls derived from direct review of the v1.0 source code (LooperEngine.cpp, PluginProcessor.cpp, PluginEditor.cpp) + verified JUCE forum sources. No generic pitfalls included.*
