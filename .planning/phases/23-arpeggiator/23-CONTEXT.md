# Phase 23: Arpeggiator — Context

**Gathered:** 2026-03-01
**Status:** Ready for planning
**Source:** Codebase scout + discuss-phase analysis

<domain>
## Phase Boundary

The arpeggiator DSP, UI, and APVTS parameters are **already fully implemented** in the codebase (PluginProcessor.cpp lines ~1245–1424, PluginEditor). Four of five success criteria already pass.

This phase delivers ONE targeted fix plus a smoke-test checkpoint:

1. **ARP-03 bar-boundary sync** — When arp is enabled while the DAW is already playing, `arpWaitingForPlay_` must release on the next bar boundary (PPQ divisible by beats-per-bar), not just on the next `dawJustStarted` event. Currently the arm-and-wait logic only releases when the clock transitions from stopped to playing.

2. **Smoke test** — Verify all 6 ARP requirements pass in DAW MIDI monitor.

</domain>

<decisions>
## Implementation Decisions

### ARP-03 Fix Approach
- When `arpOn && !prevArpOn_ && clockRunning` (arp just enabled while clock is already running), set `arpWaitingForPlay_ = true`
- On each processBlock while `arpWaitingForPlay_` is true AND clock is running, check if the current audio block crosses a bar boundary: `floor((ppqPos + beatsThisBlock) / beatsPerBar) > floor(ppqPos / beatsPerBar)` — if yes, clear the flag and let the arp fire
- `beatsPerBar` comes from `AudioPlayHead::CurrentPositionInfo::timeSigNumerator` (already available in processBlock via the position info)
- The existing `clockStarted` path (DAW play button pressed while armed) must still work — `arpWaitingForPlay_ = false` on `clockStarted` is preserved
- When DAW sync is OFF (looper clock), bar boundary = looper cycle boundary (looper beat wraps to 0) — existing `looperJustStarted` covers this case already, no change needed

### What Is NOT Changing
- All 7 order patterns (Up, Down, Up+Down, Down+Up, Outer-In, Inner-Out, Random) — keep as-is (exceeds spec, that's fine)
- All 6 rate subdivisions including triplets — keep as-is
- Gate Length integration — already correct
- `effectiveChannel()` routing — already correct
- Step reset to 0 on toggle-off — already correct

### Claude's Discretion
- Where in processBlock to compute beatsPerBar (read from position info at block start alongside ppqPos/isDawPlaying)
- Whether to cache timeSigNumerator as a member or read it fresh each block (fresh read is fine; it rarely changes mid-session)

</decisions>

<specifics>
## Specific Implementation Notes

**Current arm-and-wait code (PluginProcessor.cpp ~1256–1261):**
```cpp
if (arpOn && !prevArpOn_ && clockRunning)
    arpWaitingForPlay_ = true;   // just enabled while clock rolling — arm
if (!arpOn)
    arpWaitingForPlay_ = false;  // disabled — reset
if (clockStarted)
    arpWaitingForPlay_ = false;  // clock just started — launch
```

**Required addition:** After the `clockStarted` check, add a bar-boundary release:
```cpp
if (arpWaitingForPlay_ && arpSyncOn && isDawPlaying && ppqPos >= 0.0) {
    const int beatsPerBar = timeSigNumerator; // from AudioPlayHead position info
    const long long barAtStart = (long long)(ppqPos / beatsPerBar);
    const long long barAtEnd   = (long long)((ppqPos + beatsThisBlock) / beatsPerBar);
    if (barAtEnd > barAtStart)
        arpWaitingForPlay_ = false;
}
```

**ARP-06 requirement** (not in success criteria but in req list): Verify what ARP-06 specifies in REQUIREMENTS.md before planning.

</specifics>

<deferred>
## Deferred Ideas

- Octave range cycling (arp spans 2+ octaves) — future milestone
- Per-voice hold sustain (overlapping arp notes) — future milestone
- External MIDI input feeding arpeggiator — future milestone

</deferred>

---

*Phase: 23-arpeggiator*
*Context gathered: 2026-03-01 via codebase scout + discuss-phase analysis*
