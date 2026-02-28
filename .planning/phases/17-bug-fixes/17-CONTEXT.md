# Phase 17: Bug Fixes - Context

**Gathered:** 2026-02-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Fix two known correctness defects — looper wrong start-position after record cycle, and plugin crash on PS4 BT reconnect. No new capabilities. Both fixes must leave all existing tests passing.

</domain>

<decisions>
## Implementation Decisions

### Looper Restart Point (BUG-01)
- After recording ends, loop restarts at **beat 0 always** — discard the sub-block overshoot
- Predictable, grid-aligned feel; consistent with how most hardware loopers behave
- Root fix: replace `loopStartPpq_ = -1.0` with `loopStartPpq_ = p.ppqPosition - overshoot` where `overshoot = recordedBeats_ - loopLen`; set `internalBeat_ = 0.0` (free-running mode stays at 0)
- DAW sync mode: after recording ends, `loopStartPpq_` must be set to the ppq that corresponds to beat 0 of the loop — NOT -1.0 which triggers incorrect `anchorToBar()` re-anchoring on next block

### BT Reconnect Crash (BUG-02)
- Instant resume on reconnect — no extra UI state beyond existing status label update
- Fix 1: Guard `SDL_CONTROLLERDEVICEADDED` with `!controller_` before calling `tryOpenController()` — prevents double-open during BT enumeration
- Fix 2: Guard `SDL_CONTROLLERDEVICEREMOVED` with instance-ID comparison — `SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == ev.cdevice.which` — prevents closing wrong device
- Optional deferred-open: set `pendingReopenTick_` flag, skip one timer tick before opening — reduces risk of SDL_GameControllerOpen() during active BT enumeration

### Verification Gate
- pluginval level 5 must pass with no new failures
- Manual smoke test required in plan:
  1. Record a 4-bar loop, let it play back 3+ cycles — confirm beat 1 stays grid-locked
  2. Disconnect PS4 via BT (hold PS button > "Turn Off"), wait 5s, reconnect — confirm no crash and controller resumes

</decisions>

<code_context>
## Existing Code Insights

### BUG-01 — LooperEngine::process() (LooperEngine.cpp lines 753–764)
- `recordedBeats_` accumulates block sizes during recording; when `>= loopLen` recording auto-stops
- Current code: `internalBeat_ = 0.0; loopStartPpq_ = -1.0; recordedBeats_ = 0.0;`
- `loopStartPpq_ = -1.0` triggers `anchorToBar(ppqPosition, beatsPerBar())` on NEXT block
- `anchorToBar` floors to nearest bar boundary BEFORE current ppq — not where loop beat 0 actually is
- Fix location: the `if (recordedBeats_ >= loopLen)` block; add `const double overshoot = recordedBeats_ - loopLen;` before clearing, use it to set `loopStartPpq_`

### BUG-02 — GamepadInput::timerCallback() (GamepadInput.cpp lines 112–115)
- `SDL_CONTROLLERDEVICEADDED` → `tryOpenController()` — no guard; double-opens on BT reconnect
- `SDL_CONTROLLERDEVICEREMOVED` → `closeController()` — no instance-ID check; can close wrong device
- `tryOpenController()` is already safe for null `controller_` but is called unconditionally
- `SDL_GameControllerGetJoystick()` + `SDL_JoystickInstanceID()` — both in SDL2 2.30.9, no new deps
- Existing fallback poll at lines 120–121 (`SDL_GameControllerGetAttached`) is correct; keep it

### Established Patterns
- All atomics follow `std::memory_order_relaxed` for audio-thread-only state, `acq_rel` for cross-thread flags
- `ASSERT_AUDIO_THREAD()` macro in LooperEngine — keep all process() changes on audio thread
- No new member variables needed for looper fix; `pendingReopenTick_` atomic bool needed for BT deferred-open (optional but safer)

### Integration Points
- `LooperEngine::process()` — modify the recording auto-stop block only
- `GamepadInput::timerCallback()` — modify the SDL event loop only (lines 112–115)
- No changes to PluginProcessor, PluginEditor, or any other file expected

</code_context>

<specifics>
## Specific Requirements

- BUG-01 must be fixed before Phase 22 (LFO Recording) — looper anchor drift would corrupt LFO recording seams
- Both fixes in one phase (Phase 17) so pluginval + smoke test covers both together
- No behaviour changes beyond the two bugs — all other looper and gamepad behaviour stays identical

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 17-bug-fixes*
*Context gathered: 2026-02-28*
