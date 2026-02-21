# Pitfalls Research

**Domain:** JUCE MIDI Plugin Development
**Researched:** 2026-02-21
**Confidence:** HIGH

## Critical Pitfalls

### Pitfall 1: Memory Allocation in processBlock

**What goes wrong:**
Real-time audio thread cannot allocate memory. Any `new`, `malloc`, `std::vector::push_back`, or string operations in `processBlock()` causes audio glitches, xruns, or crashes.

**Why it happens:**
The audio callback runs on a priority real-time thread. Memory allocation can block (page fault, heap lock) causing buffer underruns. JUCE's `DBG()` macro also allocates.

**How to avoid:**
- Pre-allocate all buffers in `prepareToPlay()`
- Use fixed-size buffers, pools, or lock-free rings
- Never use `DBG()` in processBlock - use `jassert` or trace conditional logging
- Avoid `std::vector` that grows - pre-size with `.resize()`

**Warning signs:**
- Audio clicks/pops during playback
- Crashes when buffer size changes
- "Heap allocation in real-time context" assertions

**Phase to address:** Implementation - Audio Engine

---

### Pitfall 2: Thread-Unsafe Parameter Access

**What goes wrong:**
Parameters accessed from both UI thread (slider changes) and audio thread (processBlock) without synchronization causes race conditions, stale values, or crashes.

**Why it happens:**
`parameterChanged()` callbacks can fire from ANY thread (UI or audio), depending on the DAW. Developers assume they're always on message thread.

**How to avoid:**
- Use `AudioProcessorValueTreeState` with `broadcastChangeOnlyIfNotAlreadyArmed` or similar
- In parameter callbacks: only set atomic flags or trivial values
- Never call `repaint()` or do heavy processing in parameter callbacks
- Use `getRawParameterValue()` for lock-free read in processBlock

**Warning signs:**
- Crashes when moving sliders during playback
- Parameter values "stuck" or jumping unexpectedly
- Different behavior in different DAWs

**Phase to address:** Implementation - Parameter System

---

### Pitfall 3: Incorrect MIDI Buffer Timestamps

**What goes wrong:**
MIDI events fire at wrong times because sample positions within the buffer are ignored or mishandled.

**Why it happens:**
JUCE's `MidiBuffer` stores events with sample-position timestamps. Many developers iterate with `for (auto m : buffer)` and ignore the timestamp, causing all events to fire immediately.

**How to avoid:**
```cpp
for (auto metadata : buffer)
{
    auto message = metadata.getMessage();
    auto sampleOffset = metadata.samplePosition;
    // Schedule at correct sample position
}
```
- Understand that timestamp 0 = start of current block
- For precise timing, schedule events in future blocks based on ` BPM * sampleRate`

**Warning signs:**
- MIDI notes play early or late
- Arpeggiator/sequencer timing drifts
- Different timing in different DAWs

**Phase to address:** Implementation - MIDI Processing

---

### Pitfall 4: Static Memory / Singletons in Plugins

**What goes wrong:**
Static variables or singletons cause state to be shared between plugin instances, or crash when multiple instances exist in the same process.

**Why it happens:**
DAWs can load multiple instances of the same plugin. Static memory is shared across all instances in the same process. Different instances may run in the same or different processes.

**How to avoid:**
- Never use `static` variables for instance state
- Never use singletons for plugin state
- Store all state in member variables of `AudioProcessor` subclass
- Use `juce::Atomic` for shared flags if needed

**Warning signs:**
- Multiple instances share state (presets affect each other)
- Plugin crashes when second instance loads
- State persists incorrectly after duplicate

**Phase to address:** Architecture - Instance Model

---

### Pitfall 5: Plugin State Not Restoring

**What goes wrong:**
Plugin parameters reset to defaults when reopening a saved DAW project, or state is corrupted.

**Why it happens:**
- `getStateInformation()` not returning correct data
- `setStateInformation()` not properly restoring all state
- Forgetting to handle the "empty state" (first load) case
- Versioning issues when state format changes

**How to avoid:**
- Use `AudioProcessorValueTreeState` for automatic state management
- Implement proper version checking in `setStateInformation()`
- Test: save preset, reload, close DAW, reopen - verify state persists
- Handle both full state restore and individual parameter restore paths

**Warning signs:**
- DAW project reload shows default plugin state
- Presets don't restore correctly
- Console warnings about state restoration failures

**Phase to address:** Implementation - State Management

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Using `new` in processBlock | Simpler code | Audio glitches, crashes | NEVER |
| Mutex in processBlock | "Thread-safe" feeling | Deadlocks, xruns | NEVER |
| Direct MidiInput in plugin | Direct hardware access | Doesn't work in most hosts | Only for standalone |
| Skip pluginval testing | Faster initial dev | Hidden bugs, crashes at user site | NEVER |
| Hardcoded buffer sizes | Simplicity | Breaks with different buffer configs | Never |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| DAW Transport | Assuming `getPosition().getBPM()` is reliable | Cache BPM, handle fallback, check `getTimeSignature()` |
| MIDI Clock | Not handling start/stop/continue messages | Implement full MIDI clock state machine |
| Host MIDI Input | Trying to open MidiInput device in plugin | Use `processBlock` MidiBuffer from host |
| Plugin Format | Testing only one format | Test VST3, AU, AUv3 - each has quirks |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Atomic dereferencing in tight loop | CPU spikes, xruns | Cache atomics once per block | High voice count |
| Lock-free queue abuse | Sporadic crashes | Use JUCE's MessageQueue, careful design | Complex MIDI routing |
| UI updates in audio callback | Crashes, deadlocks | Use AsyncUpdater, change listeners only | Always |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| MIDI learn without validation | Unexpected note storms | Validate all incoming MIDI ranges |
| File I/O in plugin | Security sandbox blocks | JUCE handles this, but test with sandbox enabled |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| No visual feedback on MIDI out | User doesn't know if plugin works | LED / activity indicator for MIDI activity |
| Hidden critical parameters | Confusing workflow | Group related params, use labeled sections |
| No preset system | Can't save work | Implement bank/preset system early |
| Different UI per DAW | Inconsistent experience | Test in multiple DAWs, minimum viable UI |

## "Looks Done But Isn't" Checklist

- [ ] **MIDI Output:** Plugin appears to work but doesn't send to all DAWs — verify VST3 MIDI output, AUv3 MIDI input/output bus
- [ ] **BPM Sync:** Shows BPM but drifts — verify against transport, handle time signature changes
- [ ] **State Save:** Preset saves but doesn't reload in new session — test full DAW project save/load cycle
- [ ] **Multichannel:** Appears to work in mono — test stereo/MIDI multi-channel routing
- [ ] **Latency:** No latency compensation — implement `setLatencySamples()` for MIDI generators

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Memory alloc in processBlock | MEDIUM | Add prepareToPlay buffer allocation, review all loops |
| Thread-unsafe params | HIGH | Audit all parameter access, add atomics, refactor callbacks |
| Static memory | HIGH | Remove all statics, move to member variables |
| State not restoring | MEDIUM | Add logging to get/setStateInformation, test edge cases |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Memory allocation | Implementation - Audio Engine | Test with small buffer (64 samples), check for xruns |
| Thread safety | Implementation - Parameter System | Move sliders during playback in multiple DAWs |
| MIDI timestamps | Implementation - MIDI Processing | Compare with external MIDI monitor, test tempo changes |
| Static memory | Architecture - Instance Model | Load 2+ instances, verify independent state |
| State restoration | Implementation - State Management | Save project, close DAW, reopen, verify all params |
| Plugin validation | Pre-Release - Validation | Run pluginval with strict settings |
| DAW sync | Implementation - Transport Sync | Test in Ableton, Logic, Reaper, Pro Tools |

## Sources

- JUCE Forum: #1 most common programming mistake (2018) — https://forum.juce.com/t/1-most-common-programming-mistake-that-we-see-on-the-forum/26013
- JUCE Forum: Thread safety discussions — https://forum.juce.com/t/preparetoplay-and-processblock-thread-safety/32193
- JUCE Forum: MIDI buffer timing — https://forum.juce.com/t/using-timestamps-of-midi-messages-in-processblock/33957
- Melatonin: Big list of JUCE tips and tricks (2024) — https://melatonin.dev/blog/big-list-of-juce-tips-and-tricks/
- Melatonin: Pluginval is a plugin dev's best friend — https://melatonin.dev/blog/pluginval-is-a-plugin-devs-best-friend/
- JUCE Docs: Handling MIDI events — https://juce.com/tutorials/tutorial_handling_midi_events/
- JUCE Forum: Plugin state issues — https://forum.juce.com/t/plugin-state-is-not-restored-from-a-valuetree/51638

---

*Pitfalls research for: ChordJoystick - JUCE MIDI Plugin*
*Researched: 2026-02-21*
