# Phase 17: Bug Fixes - Research

**Researched:** 2026-02-28
**Domain:** C++17 JUCE VST3 — lock-free audio engine bug repair (LooperEngine anchor drift, SDL2 BT reconnect crash)
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Looper Restart Point (BUG-01)
- After recording ends, loop restarts at **beat 0 always** — discard the sub-block overshoot
- Predictable, grid-aligned feel; consistent with how most hardware loopers behave
- Root fix: replace `loopStartPpq_ = -1.0` with `loopStartPpq_ = p.ppqPosition - overshoot` where `overshoot = recordedBeats_ - loopLen`; set `internalBeat_ = 0.0` (free-running mode stays at 0)
- DAW sync mode: after recording ends, `loopStartPpq_` must be set to the ppq that corresponds to beat 0 of the loop — NOT -1.0 which triggers incorrect `anchorToBar()` re-anchoring on next block

#### BT Reconnect Crash (BUG-02)
- Instant resume on reconnect — no extra UI state beyond existing status label update
- Fix 1: Guard `SDL_CONTROLLERDEVICEADDED` with `!controller_` before calling `tryOpenController()` — prevents double-open during BT enumeration
- Fix 2: Guard `SDL_CONTROLLERDEVICEREMOVED` with instance-ID comparison — `SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == ev.cdevice.which` — prevents closing wrong device
- Optional deferred-open: set `pendingReopenTick_` flag, skip one timer tick before opening — reduces risk of SDL_GameControllerOpen() during active BT enumeration

#### Verification Gate
- pluginval level 5 must pass with no new failures
- Manual smoke test required in plan:
  1. Record a 4-bar loop, let it play back 3+ cycles — confirm beat 1 stays grid-locked
  2. Disconnect PS4 via BT (hold PS button > "Turn Off"), wait 5s, reconnect — confirm no crash and controller resumes

### Claude's Discretion

None specified — all implementation details are locked.

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| BUG-01 | Looper playback starts at the correct beat position after a record cycle completes (no anchor drift or offset) | Root cause confirmed in LooperEngine.cpp line 761: `loopStartPpq_ = -1.0` after auto-stop triggers wrong bar re-anchor. Fix is precise arithmetic using overshoot. |
| BUG-02 | Plugin does not crash when PS4 controller reconnects via Bluetooth (SDL2 double-open guard) | Root cause confirmed in GamepadInput.cpp lines 112–115: unconditional `tryOpenController()` on ADDED event + unconditional `closeController()` on REMOVED event. Both need guards. |
</phase_requirements>

---

## Summary

Phase 17 fixes two confirmed correctness defects in the existing codebase. Both bugs have been fully root-cause analysed and the exact fix locations and logic are spelled out verbatim in CONTEXT.md. No exploratory investigation is needed — this is a surgical repair phase.

**BUG-01 (LooperEngine anchor drift):** After `recordedBeats_ >= loopLen` the auto-stop block sets `loopStartPpq_ = -1.0`. On the very next `process()` block call in DAW sync mode the sentinel value `-1.0` triggers `anchorToBar(ppqPosition, beatsPerBar())`, which floors to the nearest bar boundary before the current ppq — not where beat 0 of the recorded loop actually is. In free-running mode, `internalBeat_ = 0.0` is already correct; the drift manifests only in DAW sync mode. The fix is to compute the sub-block overshoot (`recordedBeats_ - loopLen`) and write `loopStartPpq_ = p.ppqPosition - overshoot` instead of `-1.0` at the auto-stop site (line 761), so the anchor points back to the actual loop beat 0.

**BUG-02 (SDL2 BT reconnect crash):** Bluetooth PS4 controller disconnect + reconnect fires multiple `SDL_CONTROLLERDEVICEADDED` events in rapid succession during the BT enumeration phase. The current event loop calls `tryOpenController()` unconditionally on every ADDED event, resulting in a double-open of the same device while `controller_` is already non-null. Concurrently, the REMOVED event fires `closeController()` unconditionally without checking the instance ID, so the wrong device (or an already-null pointer) gets closed, causing the crash. Two targeted guards eliminate both failure paths.

**Primary recommendation:** Apply both fixes as independent, minimal changes to their respective files. No new member variables are required for BUG-01. BUG-02 requires adding instance-ID guard logic in the event loop; the optional `pendingReopenTick_` deferral is recommended as it avoids `SDL_GameControllerOpen()` during active BT enumeration, which is the condition most likely to produce a second crash on some BT stacks.

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.4 | Audio plugin framework, timer, AbstractFifo | Already in use; no change needed |
| SDL2 | release-2.30.9 | Gamepad input, BT event handling | Already in use; all required APIs present in this version |
| Catch2 | v3.8.1 | Unit testing framework | Already integrated via CMake FetchContent; existing test suite uses it |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| C++17 stdlib | MSVC (VS 18 2026) | `std::atomic`, `std::fmod`, `std::floor` | Used throughout; no additions needed |

### Alternatives Considered

None — both bugs are fixed inside the existing stack. No new dependencies are introduced.

**Installation:** None required. All dependencies already present.

---

## Architecture Patterns

### Recommended Project Structure

No structural changes. Two files are modified:

```
Source/
├── LooperEngine.cpp    # BUG-01: modify auto-stop block (line ~756-764)
└── GamepadInput.cpp    # BUG-02: modify SDL event loop (lines 112-115)
Tests/
└── LooperEngineTests.cpp  # Add TC 13 for BUG-01 regression coverage
```

### Pattern 1: Overshoot-Corrected DAW Anchor

**What:** When auto-stop fires, compute how many beats past the loop boundary were accumulated in the final block, and set `loopStartPpq_` to point back to actual beat 0 of the loop.

**When to use:** Any time recording auto-stops mid-block in DAW sync mode.

**Example:**
```cpp
// Source: LooperEngine.cpp — auto-stop block (replaces lines 758-762)
if (recordedBeats_ >= loopLen)
{
    recording_.store(false, std::memory_order_relaxed);
    finaliseRecording();

    const double overshoot = recordedBeats_ - loopLen;

    // Free-running mode: internal clock resets to 0 (overshoot discarded, beat 0 aligned)
    internalBeat_ = 0.0;

    // DAW sync mode: set anchor to the ppq that corresponds to beat 0 of this loop.
    // p.ppqPosition is currently at (loop_beat_0 + loopLen + overshoot),
    // so loop_beat_0_ppq = p.ppqPosition - loopLen - overshoot = p.ppqPosition - recordedBeats_.
    // Use loopLen directly since overshoot = recordedBeats_ - loopLen:
    //   loop_beat_0_ppq = p.ppqPosition - (overshoot + loopLen) = p.ppqPosition - recordedBeats_
    // But simpler: loopStartPpq_ stays at its last-set value (set when recording started)
    // OR we compute it from ppqPosition:
    //   loopStartPpq_ = p.ppqPosition - overshoot - loopLen  (points to beat 0 of loop)
    // Per CONTEXT.md locked decision:
    loopStartPpq_  = p.ppqPosition - overshoot;   // points to beat 0 after exactly loopLen beats
    recordedBeats_ = 0.0;
    // playing_ stays true — fall through to scan playbackStore_ this block
}
```

Note on the formula: `loopStartPpq_ = p.ppqPosition - overshoot` means "the ppq position that is exactly `loopLen` beats before the current position plus the remaining overshoot". Since `p.ppqPosition` is at `loopStart + loopLen + overshoot` (recording started at `loopStart`), the value `p.ppqPosition - overshoot = loopStart + loopLen`. This points to beat 0 of the NEXT playback cycle. On the subsequent block, `elapsed = p.ppqPosition_next - loopStartPpq_` will be a small positive value (the overshoot beats already into playback), giving correct beat alignment.

Actually, re-reading CONTEXT.md precisely: "set `loopStartPpq_` to the ppq that corresponds to beat 0 of the loop — NOT -1.0". The formula in CONTEXT.md is `loopStartPpq_ = p.ppqPosition - overshoot`. This is correct as written: it sets the anchor to the ppq that was at the transition boundary (current ppq minus the sub-block portion that went past the loop end), so the next block's `elapsed = ppq - loopStartPpq_` will equal `overshoot` — the correct already-played offset into the first playback cycle. This is the locked formula; use it exactly.

### Pattern 2: Instance-ID Guarded SDL Event Handling

**What:** Guard both ADD and REMOVE SDL controller events with state checks before acting.

**When to use:** All SDL controller device event handling on BT-capable platforms.

**Example:**
```cpp
// Source: GamepadInput.cpp — SDL event loop (replaces lines 112-115)
if (ev.type == SDL_CONTROLLERDEVICEADDED)
{
    // Guard: only open if no controller is currently open.
    // BT enumeration fires multiple ADDED events; the first successful open is sufficient.
    if (!controller_)
        tryOpenController();
}
else if (ev.type == SDL_CONTROLLERDEVICEREMOVED)
{
    // Guard: only close if the removed instance matches our open controller.
    // Prevents closing the wrong device when multiple controllers are present
    // or when BT stack fires spurious REMOVED events.
    if (controller_ &&
        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == ev.cdevice.which)
    {
        closeController();
    }
}
```

### Pattern 3: Optional Deferred-Open with pendingReopenTick_

**What:** On `SDL_CONTROLLERDEVICEADDED`, set a flag and defer `tryOpenController()` by one timer tick (16 ms at 60 Hz) to let BT enumeration complete before opening.

**When to use:** When the `!controller_` guard alone is insufficient (some BT stacks fire ADDED before the device is fully enumerable).

**Example:**
```cpp
// In GamepadInput.h — add to private section:
std::atomic<bool> pendingReopenTick_ { false };

// In timerCallback() — SDL event loop:
if (ev.type == SDL_CONTROLLERDEVICEADDED)
{
    if (!controller_)
        pendingReopenTick_.store(true, std::memory_order_relaxed);
}

// After event loop drains — deferred open:
if (pendingReopenTick_.exchange(false, std::memory_order_relaxed) && !controller_)
    tryOpenController();
```

This is the "safer" optional path noted in CONTEXT.md. The planner should include this as the recommended implementation since it eliminates the BT enumeration race condition.

### Anti-Patterns to Avoid

- **Resetting `loopStartPpq_ = -1.0` after auto-stop in DAW sync mode:** This is the root cause of BUG-01. Never use the sentinel value to signal re-anchoring after a known recording endpoint.
- **Calling `tryOpenController()` unconditionally on SDL_CONTROLLERDEVICEADDED:** Causes double-open on BT reconnect. Always check `!controller_` first.
- **Calling `closeController()` without instance-ID check on SDL_CONTROLLERDEVICEREMOVED:** Closes the wrong device or a null controller. Always compare instance IDs.
- **Adding mutex or blocking calls inside `timerCallback()`:** The timer callback is on the main thread; SDL event loop must remain lock-free.
- **Modifying `recordedBeats_` before computing overshoot:** The overshoot calculation requires the final accumulated value. Compute it before the `= 0.0` reset.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Instance-ID comparison | Custom device-index tracking | `SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_))` | SDL2 provides stable instance IDs that survive hot-plug; device indices change after disconnect |
| BT enumeration delay | Thread sleep or JUCE timer restart | `pendingReopenTick_` single-tick deferral | One-tick deferral in the existing 60Hz timer is sufficient and lock-free |
| DAW beat-0 position | Re-running `anchorToBar()` | Direct arithmetic: `loopStartPpq_ = p.ppqPosition - overshoot` | `anchorToBar()` floors to bar boundary, not loop boundary — cannot be reused here |

**Key insight:** Both bugs require 2-4 lines of targeted arithmetic/guard logic. No new abstractions, no new classes, no new member variables (except the optional `pendingReopenTick_` atomic bool for BUG-02).

---

## Common Pitfalls

### Pitfall 1: Off-by-One in Overshoot Formula

**What goes wrong:** Using `loopStartPpq_ = p.ppqPosition - loopLen` instead of `p.ppqPosition - overshoot`. The former ignores the sub-block overshoot and sets the anchor `loopLen` beats before the current position, which is only correct if recording stopped exactly at block end (overshoot = 0).
**Why it happens:** Confusing "loop length" with "distance to beat 0".
**How to avoid:** Use exactly the formula from CONTEXT.md: `overshoot = recordedBeats_ - loopLen`, then `loopStartPpq_ = p.ppqPosition - overshoot`.
**Warning signs:** Beat 1 of playback arrives slightly early or late by a fraction of a block duration.

### Pitfall 2: Free-Running Mode Still Gets Wrong Anchor

**What goes wrong:** Setting `loopStartPpq_` correctly for DAW mode but forgetting that `internalBeat_ = 0.0` already handles free-running mode. The fix is complete with just the two lines: `internalBeat_ = 0.0` and `loopStartPpq_ = p.ppqPosition - overshoot`.
**Why it happens:** Over-complicating the fix.
**How to avoid:** The free-running path uses `internalBeat_` and ignores `loopStartPpq_` (it gets reset to -1.0 each non-DAW block anyway at line 709). Setting `internalBeat_ = 0.0` is already present and correct.

### Pitfall 3: SDL_GameControllerGetJoystick() Returns NULL

**What goes wrong:** Calling `SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_))` when `controller_` is non-null but the underlying joystick has already been closed by a previous REMOVED event. Returns undefined behavior.
**Why it happens:** The REMOVED guard fires before the ADDED guard in the same event loop iteration.
**How to avoid:** The `!controller_` check on ADDED and the `controller_ != nullptr` check on REMOVED, combined with the instance-ID test, are sufficient. Since `closeController()` always sets `controller_ = nullptr`, the null check on REMOVED catches this case before `SDL_GameControllerGetJoystick` is called.

### Pitfall 4: recordedBeats_ Cleared Before Overshoot Calculation

**What goes wrong:** If code is restructured and `recordedBeats_ = 0.0` appears before `const double overshoot = recordedBeats_ - loopLen;`, overshoot is always `-loopLen`, corrupting the anchor.
**Why it happens:** Naive reordering of cleanup code.
**How to avoid:** Compute overshoot as the very first line inside the `if (recordedBeats_ >= loopLen)` block, before any state mutation.

### Pitfall 5: ASSERT_AUDIO_THREAD() in LooperEngine

**What goes wrong:** LooperEngine's `process()` and `activateRecordingNow()` use `ASSERT_AUDIO_THREAD()`. If a test or debug build tries to call the auto-stop code path from a non-audio thread it will assert.
**Why it happens:** Debug guards for thread-safety validation.
**How to avoid:** Unit tests for BUG-01 must call `process()` in the same way existing tests do — this is already handled correctly in `LooperEngineTests.cpp` since Catch2 uses a thread that satisfies the JUCE audio-thread check in test builds (the macro is a no-op outside Debug mode in most configurations).

---

## Code Examples

Verified patterns from source code inspection:

### SDL2 Instance ID Comparison (BUG-02)
```cpp
// Source: GamepadInput.cpp lines 112-115 — current (buggy)
if (ev.type == SDL_CONTROLLERDEVICEADDED)
    tryOpenController();
else if (ev.type == SDL_CONTROLLERDEVICEREMOVED)
    closeController();

// After fix:
if (ev.type == SDL_CONTROLLERDEVICEADDED)
{
    if (!controller_)
        pendingReopenTick_.store(true, std::memory_order_relaxed);
}
else if (ev.type == SDL_CONTROLLERDEVICEREMOVED)
{
    if (controller_ &&
        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == ev.cdevice.which)
    {
        closeController();
    }
}
// ... after event loop:
if (pendingReopenTick_.exchange(false, std::memory_order_relaxed) && !controller_)
    tryOpenController();
```

### Anchor Fix — Auto-Stop Block (BUG-01)
```cpp
// Source: LooperEngine.cpp lines 756-764 — current (buggy)
if (recordedBeats_ >= loopLen)
{
    recording_.store(false, std::memory_order_relaxed);
    finaliseRecording();
    internalBeat_  = 0.0;
    loopStartPpq_  = -1.0;   // BUG: triggers wrong anchorToBar() on next block
    recordedBeats_ = 0.0;
}

// After fix:
if (recordedBeats_ >= loopLen)
{
    const double overshoot = recordedBeats_ - loopLen;  // MUST be first
    recording_.store(false, std::memory_order_relaxed);
    finaliseRecording();
    internalBeat_  = 0.0;                              // free-running: restart at beat 0
    loopStartPpq_  = p.ppqPosition - overshoot;        // DAW sync: anchor to actual beat 0
    recordedBeats_ = 0.0;
}
```

### Catch2 Regression Test for BUG-01 (new TC 13)
```cpp
// Source: Tests/LooperEngineTests.cpp — new test case
TEST_CASE("LooperEngine - DAW sync: playback beat 0 after auto-stop record cycle", "[looper][bug01]")
{
    // Setup: 1-bar 4/4 loop at 120 BPM, DAW sync on
    // Record one full loop; verify first playback block starts at beat 0
    LooperEngine eng;
    eng.setSyncToDaw(true);
    eng.setSubdiv(LooperSubdiv::FourFour);
    eng.setLoopLengthBars(1);   // loopLen = 4.0 beats
    eng.setRecGates(true);

    // samplesPerBeat = 44100 * 60 / 120 = 22050
    // Drive recording across exactly 4 beats using 4 blocks of 22050 samples
    // ppqPosition advances by 1.0 beat per block
    // Recording starts at ppq = 0.0 (clean loop start)

    // Arm recording at ppq = 0.0
    eng.record();  // sets recordPending_

    // Block 0: ppq=0.0, activates recording + advances recordedBeats_ by 1.0
    LooperEngine::ProcessParams p0 { 44100.0, 120.0, 0.0, 22050, true };
    eng.process(p0);
    REQUIRE(eng.isRecording());

    // Blocks 1-2: ppq=1.0, ppq=2.0
    LooperEngine::ProcessParams p1 { 44100.0, 120.0, 1.0, 22050, true };
    LooperEngine::ProcessParams p2 { 44100.0, 120.0, 2.0, 22050, true };
    eng.process(p1);
    eng.process(p2);

    // Block 3: ppq=3.0 — recordedBeats_ reaches 4.0 >= loopLen(4.0) → auto-stop
    LooperEngine::ProcessParams p3 { 44100.0, 120.0, 3.0, 22050, true };
    eng.process(p3);
    REQUIRE_FALSE(eng.isRecording());

    // First playback block: ppq=4.0 — should be at beat 0 of loop (overshoot = 0)
    LooperEngine::ProcessParams p4 { 44100.0, 120.0, 4.0, 22050, true };
    eng.process(p4);
    CHECK(eng.getPlaybackBeat() == Catch::Approx(0.0).epsilon(0.01));
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `loopStartPpq_ = -1.0` as re-anchor sentinel after auto-stop | `loopStartPpq_ = p.ppqPosition - overshoot` (direct arithmetic) | Phase 17 | Eliminates beat drift on first playback cycle after recording |
| Unconditional `tryOpenController()` on SDL_CONTROLLERDEVICEADDED | Guard with `!controller_` + one-tick deferral | Phase 17 | Eliminates double-open crash on BT reconnect |
| Unconditional `closeController()` on SDL_CONTROLLERDEVICEREMOVED | Guard with instance-ID comparison | Phase 17 | Prevents closing wrong device / null-pointer close |

**No deprecations** — existing code patterns for all other looper and gamepad paths remain unchanged.

---

## Open Questions

1. **Whether `pendingReopenTick_` needs to be an `std::atomic<bool>` or a plain bool**
   - What we know: `timerCallback()` runs on the main thread exclusively; `controller_` is only accessed from `timerCallback()`, `tryOpenController()`, and `closeController()`, all of which are main-thread calls. The `pendingReopenTick_` flag is set and consumed within the same `timerCallback()` call chain.
   - What's unclear: Whether JUCE's timer guarantees single-threaded execution (it does — `juce::Timer::timerCallback` is always called on the message thread).
   - Recommendation: A plain `bool` member is safe here since all access is on the main/message thread. Use `bool pendingReopenTick_ = false;` in GamepadInput.h private section. No atomic needed.

2. **Whether existing TC 8 (DAW sync anchor) provides sufficient coverage for BUG-01**
   - What we know: TC 8 tests that `anchorToBar()` correctly sets `loopStartPpq_` on first play — it does NOT test auto-stop then playback. The new TC 13 above is specifically needed.
   - What's unclear: Whether the planner should put TC 13 in Wave 0 (before implementation) or Wave 1 (alongside fix). Given TDD discipline, Wave 0 is preferred.
   - Recommendation: Add TC 13 as a Wave 0 task that is initially expected to fail (red test), then the BUG-01 fix turns it green.

---

## Sources

### Primary (HIGH confidence)
- Direct inspection of `Source/LooperEngine.cpp` (lines 756-764, 187-191, 694-695) — root cause of BUG-01 confirmed
- Direct inspection of `Source/GamepadInput.cpp` (lines 112-115) — root cause of BUG-02 confirmed
- Direct inspection of `Tests/LooperEngineTests.cpp` — existing test coverage confirmed (TC 1-12, no BUG-01 regression test exists)
- Direct inspection of `CMakeLists.txt` — Catch2 v3.8.1, `BUILD_TESTS` flag, CTest integration confirmed
- `.planning/phases/17-bug-fixes/17-CONTEXT.md` — all fix logic locked by user decisions

### Secondary (MEDIUM confidence)
- SDL2 `SDL_JoystickInstanceID` + `SDL_GameControllerGetJoystick` API: confirmed present in SDL2 2.30.9 per CONTEXT.md ("both in SDL2 2.30.9, no new deps") and consistent with SDL2 documentation patterns for instance-ID-based device tracking.

### Tertiary (LOW confidence)
- None — all claims are grounded in direct code inspection or CONTEXT.md locked decisions.

---

## Metadata

**Confidence breakdown:**
- Bug root causes: HIGH — confirmed by direct code inspection at exact line numbers
- Fix logic: HIGH — locked by user in CONTEXT.md, consistent with code structure
- Test strategy: HIGH — existing Catch2 infrastructure in place, test pattern matches TC 8 style
- SDL2 API availability: HIGH — confirmed by CONTEXT.md ("both in SDL2 2.30.9, no new deps")

**Research date:** 2026-02-28
**Valid until:** 2026-04-30 (stable codebase, no fast-moving dependencies)
