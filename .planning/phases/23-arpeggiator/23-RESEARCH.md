# Phase 23: Arpeggiator — Research

**Researched:** 2026-03-01
**Domain:** JUCE VST3 C++17 — arpeggiator bar-boundary sync fix + DAW smoke-test verification
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**ARP-03 Fix Approach**
- When `arpOn && !prevArpOn_ && clockRunning` (arp just enabled while clock is already running), set `arpWaitingForPlay_ = true`
- On each processBlock while `arpWaitingForPlay_` is true AND clock is running, check if the current audio block crosses a bar boundary: `floor((ppqPos + beatsThisBlock) / beatsPerBar) > floor(ppqPos / beatsPerBar)` — if yes, clear the flag and let the arp fire
- `beatsPerBar` comes from `AudioPlayHead::CurrentPositionInfo::timeSigNumerator` (already available in processBlock via the position info)
- The existing `clockStarted` path (DAW play button pressed while armed) must still work — `arpWaitingForPlay_ = false` on `clockStarted` is preserved
- When DAW sync is OFF (looper clock), bar boundary = looper cycle boundary (looper beat wraps to 0) — existing `looperJustStarted` covers this case already, no change needed

**What Is NOT Changing**
- All 7 order patterns (Up, Down, Up+Down, Down+Up, Outer-In, Inner-Out, Random) — keep as-is (exceeds spec, that's fine)
- All 6 rate subdivisions including triplets — keep as-is
- Gate Length integration — already correct
- `effectiveChannel()` routing — already correct
- Step reset to 0 on toggle-off — already correct

### Claude's Discretion
- Where in processBlock to compute beatsPerBar (read from position info at block start alongside ppqPos/isDawPlaying)
- Whether to cache timeSigNumerator as a member or read it fresh each block (fresh read is fine; it rarely changes mid-session)

### Deferred Ideas (OUT OF SCOPE)
- Octave range cycling (arp spans 2+ octaves) — future milestone
- Per-voice hold sustain (overlapping arp notes) — future milestone
- External MIDI input feeding arpeggiator — future milestone
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| ARP-01 | Arpeggiator steps through the current 4-voice chord at configurable Rate and Order, sending note-on/off on the active channel routing | Already implemented in PluginProcessor.cpp lines 1292–1470. APVTS params `arpEnabled`, `arpSubdiv`, `arpOrder` registered. UI in PluginEditor fully wired. Smoke test confirms. |
| ARP-02 | Arp Rate options: 1/4, 1/8, 1/16, 1/32 | Already implemented — `kSubdivBeats[]` at line 1341 covers 1/4, 1/8T, 1/8, 1/16T, 1/16, 1/32 (spec requires 4 of these 6). UI items confirmed in PluginEditor.cpp lines 1498–1503. |
| ARP-03 | Arp Order options: Up, Down, Up-Down, Random | Already implemented — 7 order patterns at lines 1425–1448 (Up, Down, Up+Down, Down+Up, Outer-In, Inner-Out, Random). Spec requires 4 of these 7. ONE GAP: when arp is enabled while DAW is already playing, the arm-and-wait logic only releases on `clockStarted` (DAW stop→play transition), NOT on the next bar boundary. Fix required. |
| ARP-04 | Arp uses the unified Gate Length parameter (shared with Random trigger source) | Already implemented — line 1349: `gateRatio = apvts.getRawParameterValue("gateLength")`. UI wires `arpGateTimeKnob_` to `"gateLength"` APVTS param at line 1535. |
| ARP-05 | When DAW sync is active, Arp on arms and waits for the next bar boundary before stepping begins | PARTIALLY implemented — the arm (`arpWaitingForPlay_ = true`) and the `clockStarted` release (`dawJustStarted`) paths exist. MISSING: mid-playback bar-boundary release. This is the exact fix for ARP-03. |
| ARP-06 | Arp off resets the step counter to 0 | Already implemented — at line 1334: `arpStep_ = 0` inside the `!arpOn || !arpClockOn || arpWaitingForPlay_` branch. |
</phase_requirements>

## Summary

The arpeggiator implementation is functionally complete for five of the six ARP requirements. All APVTS parameters (`arpEnabled`, `arpSubdiv`, `arpOrder`) are registered, the full step sequencer (lines 1292–1470 of PluginProcessor.cpp) handles PPQ-locked steps, looper-locked steps, and free-running steps, and the UI panel (`arpEnabledBtn_`, `arpSubdivBox_`, `arpOrderBox_`, `arpGateTimeKnob_`) is fully wired. ARP-01, ARP-02, ARP-04, and ARP-06 pass by inspection.

The single remaining gap is ARP-03/ARP-05: the `arpWaitingForPlay_` flag is set correctly when the arp is enabled while the DAW clock is already running, but it is only cleared by a `clockStarted` event (the DAW transitioning from stopped to playing). If the DAW is already playing when the user flips ARP ON, the arp stays armed forever because `clockStarted` never fires. The fix is a bar-boundary check that runs each processBlock while `arpWaitingForPlay_` is true.

The fix is one code block, approximately 7 lines, inserted at PluginProcessor.cpp line 1308 (after the `if (clockStarted) arpWaitingForPlay_ = false;` line and before `prevArpOn_ = arpOn;`). It reads `timeSigNumerator` from the JUCE 8 `PositionInfo` via `posOpt->getTimeSignature()` — a method that is already confirmed present in the JUCE 8.0.4 source at build/_deps/juce-src/modules/juce_audio_basics/audio_play_head/juce_AudioPlayHead.h line 351. Alternatively, the looper's existing `looper_.beatsPerBar()` method provides the same value from the plugin's looper subdivison setting and can be used as a fallback.

**Primary recommendation:** Add the bar-boundary release block immediately after the existing `clockStarted` release line, read `timeSigNumerator` fresh from `posOpt->getTimeSignature()` at the position-info extraction site (line 568 of PluginProcessor.cpp), and propagate it as a local `int timeSigNumerator` variable to the arm-and-wait block — then run a DAW smoke test confirming all six ARP requirements.

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE AudioPlayHead::PositionInfo | 8.0.4 | PPQ position + time signature | Already the only clock source in processBlock |
| JUCE AudioProcessorValueTreeState | 8.0.4 | APVTS param read | All existing arp params use this pattern |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| LooperEngine::beatsPerBar() | project | Looper-mode bar length in beats | Fallback when DAW sync is OFF and time sig unavailable |

**Installation:** No new dependencies. All libraries are already linked.

## Architecture Patterns

### Recommended Fix Location

The arm-and-wait code block lives at PluginProcessor.cpp lines 1293–1309:

```cpp
// ARM-AND-WAIT (existing, lines 1296–1309)
{
    const bool arpSyncOn    = looper_.isSyncToDaw();
    const bool clockRunning = arpSyncOn ? isDawPlaying : (looper_.isPlaying() || isDawPlaying);
    const bool clockStarted = arpSyncOn ? dawJustStarted : (looperJustStarted || dawJustStarted);
    if (arpOn && !prevArpOn_ && clockRunning)
        arpWaitingForPlay_ = true;   // just enabled while clock rolling — arm
    if (!arpOn)
        arpWaitingForPlay_ = false;  // disabled — reset
    if (clockStarted)
        arpWaitingForPlay_ = false;  // clock just started — launch

    // ---- INSERT HERE (after clockStarted release, before closing brace) ----
}
prevArpOn_ = arpOn;
```

### Pattern 1: JUCE 8 Time Signature Extraction

**What:** Read `timeSigNumerator` from the JUCE 8 `Optional<PositionInfo>` at the position-info extraction site, store as a local `int` for use downstream.

**When to use:** Needed once per processBlock, alongside existing `ppqPos`/`isDawPlaying`/`dawBpm` extraction.

**Example:**
```cpp
// Source: build/_deps/juce-src/modules/juce_audio_basics/audio_play_head/juce_AudioPlayHead.h line 351
// Insert at PluginProcessor.cpp line 568 alongside existing ppqPos extraction:
double ppqPos        = -1.0;
double dawBpm        = 120.0;
bool   isDawPlaying  = false;
int    timeSigNumer  = 4;    // NEW — default 4/4
if (auto* head = getPlayHead())
{
    if (auto posOpt = head->getPosition(); posOpt.hasValue())
    {
        isDawPlaying = posOpt->getIsPlaying();
        if (auto ppq = posOpt->getPpqPosition(); ppq.hasValue())
            ppqPos = *ppq;
        if (auto bpmOpt = posOpt->getBpm(); bpmOpt.hasValue())
            dawBpm = *bpmOpt;
        if (auto sig = posOpt->getTimeSignature(); sig.hasValue())  // NEW
            timeSigNumer = sig->numerator;                           // NEW
    }
}
```

### Pattern 2: Bar-Boundary Release Block

**What:** Detect when the current audio block crosses a bar boundary (PPQ advances past an integer multiple of `beatsPerBar`) and clear the arm flag.

**When to use:** Inserted inside the arm-and-wait scoped block, after the `if (clockStarted)` line.

**Example:**
```cpp
// Source: CONTEXT.md — locked implementation decision
// Inserted at PluginProcessor.cpp ~line 1308 (after clockStarted release)
if (arpWaitingForPlay_ && arpSyncOn && isDawPlaying && ppqPos >= 0.0)
{
    const double beatsPerBar = static_cast<double>(timeSigNumer);
    const long long barAtStart = static_cast<long long>(ppqPos / beatsPerBar);
    const long long barAtEnd   = static_cast<long long>((ppqPos + beatsThisBlock) / beatsPerBar);
    if (barAtEnd > barAtStart)
        arpWaitingForPlay_ = false;   // bar boundary crossed — launch
}
```

**Note on `beatsThisBlock`:** This variable is computed inside the `else` branch of the arp step sequencer (line 1354), not at the arm-and-wait site. It must be computed before the arm-and-wait block or computed inline. The formula is: `lp.bpm * (double)blockSize / (sampleRate_ * 60.0)`. `lp.bpm` and `lp` are already available at the arm-and-wait site.

### Anti-Patterns to Avoid

- **Using `looper_.beatsPerBar()` for DAW sync mode:** The looper's time signature setting (3/4, 4/4, 5/4, etc.) may differ from the DAW's time signature. When `arpSyncOn` is true, read `timeSigNumerator` from the DAW via `posOpt->getTimeSignature()` so the bar boundary matches the actual DAW grid.
- **Computing `beatsThisBlock` after the arm check:** `beatsThisBlock` is needed inside the arm block. Either compute it before the arm-and-wait scoped block, or inline it as `lp.bpm * (double)blockSize / (sampleRate_ * 60.0)`.
- **Missing the `arpSyncOn &&` guard:** The bar-boundary release must only fire in DAW sync mode. In looper-sync mode, `looperJustStarted` already handles launch correctly.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Time signature query | Custom member or APVTS param | `posOpt->getTimeSignature()` from JUCE 8 PositionInfo | DAW reports actual time sig, avoids mismatch with looper setting |
| Bar position tracking | Dedicated bar counter member | Integer division of `ppqPos / beatsPerBar` | One-liner, no state, works across loop wraps |

**Key insight:** The existing arp step-locking code (lines 1381–1387) already uses the same `floor(ppq / subdivBeats)` pattern for step counting. The bar-boundary release applies the same idiom at bar granularity.

## Common Pitfalls

### Pitfall 1: `beatsThisBlock` Not Available at Arm-and-Wait Site

**What goes wrong:** The `beatsThisBlock` variable (line 1354) is declared inside the `else` branch of `if (!arpOn || !arpClockOn || arpWaitingForPlay_)`, which is below and mutually exclusive with the arm-and-wait block.

**Why it happens:** The arm-and-wait block at lines 1293–1309 is a separate scoped block before the main arp sequencer branch.

**How to avoid:** Compute `beatsThisBlock` once before the arm-and-wait block using the same formula already used at line 1354, or inline it in the bar-boundary check: `lp.bpm * static_cast<double>(blockSize) / (sampleRate_ * 60.0)`. `lp` is available because `looper_.process(lp)` is already called by line 596.

**Warning signs:** Compiler error "undeclared identifier 'beatsThisBlock'" — fix by moving or inlining the computation.

### Pitfall 2: timeSigNumerator Not Extracted in JUCE 8 Optional API

**What goes wrong:** Attempting to read `posOpt->timeSigNumerator` (old JUCE 6/7 struct field syntax) fails to compile under JUCE 8.

**Why it happens:** JUCE 8 wraps time signature in `Optional<TimeSignature>` returned by `getTimeSignature()`. The old `CurrentPositionInfo` struct with direct fields is a compatibility shim; `PositionInfo` (the JUCE 8 type) uses method getters.

**How to avoid:** Use `if (auto sig = posOpt->getTimeSignature(); sig.hasValue()) timeSigNumer = sig->numerator;`. This matches the pattern already used in the codebase for `getPpqPosition()` and `getBpm()`.

**Warning signs:** Compile error about `timeSigNumerator` not being a member of `juce::AudioPlayHead::PositionInfo`.

### Pitfall 3: Arp Launches One Block Early on Exact Bar Boundary

**What goes wrong:** If `ppqPos` at the start of a block is exactly on a bar boundary (e.g., `ppqPos = 4.0` in 4/4), the bar-boundary check sees `barAtStart` and `barAtEnd` pointing to bars N and N+1 respectively, and launches the arp. On the next block that starts at `ppqPos = 4.0 + epsilon`, `barAtStart` and `barAtEnd` are both bar N+1, so no second false trigger. This is correct behavior.

**Why it happens:** Floating-point: `4.0 / 4.0 = 1` exactly; `(4.0 + beatsThisBlock) / 4.0 > 1`. The first block at bar 1 (ppq=0) also passes, but `arpWaitingForPlay_` is false at that point (the arp just turned on during this block, so `arpOn && !prevArpOn_` sets the flag in the same block where `ppq=0` might also satisfy the bar check — evaluate order matters).

**How to avoid:** The arm logic runs first (`arpWaitingForPlay_ = true` is set), then the bar-boundary check runs. Since these are sequential within the same block, the flag is set and then immediately checked. If the arp is enabled exactly on a bar boundary: `arpWaitingForPlay_` is set to `true`, then the bar-boundary check runs and may immediately clear it — this is the correct "launch on this bar boundary" behavior. The check `arpOn && !prevArpOn_` ensures this only happens on the enabling edge.

### Pitfall 4: Orphaned `arpGateTimeAtt_` Declaration Mismatch

**What goes wrong:** `PluginEditor.h` declares `arpGateTimeAtt_` as `std::unique_ptr<SliderAtt>` at line 358, and `PluginEditor.cpp` line 1535 creates `SliderAtt(p.apvts, "gateLength", arpGateTimeKnob_)`. The APVTS param `"arpGateTime"` was removed in Phase 20. This is already correct — the attachment binds to `"gateLength"`. No change needed.

**Warning signs:** If a future refactor renames `"gateLength"`, the arp gate knob silently breaks.

## Code Examples

Verified patterns from official sources:

### Complete Extraction Site Change (PluginProcessor.cpp ~line 568)

```cpp
// Source: juce_AudioPlayHead.h line 351 (JUCE 8.0.4) + existing project pattern
double ppqPos        = -1.0;
double dawBpm        = 120.0;
bool   isDawPlaying  = false;
int    timeSigNumer  = 4;           // NEW — beats per bar (default 4/4)
if (auto* head = getPlayHead())
{
    if (auto posOpt = head->getPosition(); posOpt.hasValue())
    {
        isDawPlaying = posOpt->getIsPlaying();
        if (auto ppq = posOpt->getPpqPosition(); ppq.hasValue())
            ppqPos = *ppq;
        if (auto bpmOpt = posOpt->getBpm(); bpmOpt.hasValue())
            dawBpm = *bpmOpt;
        if (auto sig = posOpt->getTimeSignature(); sig.hasValue())  // NEW
            timeSigNumer = sig->numerator;                          // NEW
    }
}
```

### Complete Arm-and-Wait Block with Bar-Boundary Release (PluginProcessor.cpp ~line 1293)

```cpp
// Source: CONTEXT.md locked decision + existing project pattern at lines 1293–1309
{
    const bool arpSyncOn    = looper_.isSyncToDaw();
    const bool clockRunning = arpSyncOn ? isDawPlaying
                                        : (looper_.isPlaying() || isDawPlaying);
    const bool clockStarted = arpSyncOn ? dawJustStarted
                                        : (looperJustStarted || dawJustStarted);

    if (arpOn && !prevArpOn_ && clockRunning)
        arpWaitingForPlay_ = true;    // just enabled while clock rolling — arm

    if (!arpOn)
        arpWaitingForPlay_ = false;   // disabled — reset arm state

    if (clockStarted)
        arpWaitingForPlay_ = false;   // clock just started — launch immediately

    // Bar-boundary release: fires when armed and DAW block crosses a bar line.
    // Guards: arpSyncOn (DAW sync only — looper sync uses looperJustStarted above),
    //         isDawPlaying (no bar detection while stopped),
    //         ppqPos >= 0.0 (valid position from DAW).
    if (arpWaitingForPlay_ && arpSyncOn && isDawPlaying && ppqPos >= 0.0)
    {
        const double beatsThisBlock = lp.bpm * static_cast<double>(blockSize)
                                      / (sampleRate_ * 60.0);
        const double beatsPerBar    = static_cast<double>(timeSigNumer);
        const long long barAtStart  = static_cast<long long>(ppqPos / beatsPerBar);
        const long long barAtEnd    = static_cast<long long>((ppqPos + beatsThisBlock)
                                                             / beatsPerBar);
        if (barAtEnd > barAtStart)
            arpWaitingForPlay_ = false;   // bar boundary crossed — launch
    }
}
prevArpOn_ = arpOn;
```

## Requirement Status Audit

All six ARP requirements mapped against current codebase:

| Req | Description (abbreviated) | Implementation Site | Status |
|-----|--------------------------|---------------------|--------|
| ARP-01 | Steps through 4-voice chord at Rate/Order | PluginProcessor.cpp lines 1292–1470 | IMPLEMENTED |
| ARP-02 | Rate: 1/4, 1/8, 1/16, 1/32 | `kSubdivBeats[]` line 1341 + arpSubdivBox_ items | IMPLEMENTED (6 options, spec requires 4) |
| ARP-03 | Order: Up, Down, Up-Down, Random | `switch (orderIdx)` lines 1425–1448 + arpOrderBox_ items | IMPLEMENTED (7 options, spec requires 4) |
| ARP-04 | Unified Gate Length param | `gateRatio` from `"gateLength"` line 1349 + arpGateTimeAtt_ to `"gateLength"` | IMPLEMENTED |
| ARP-05 | DAW sync on → arm, launch on next bar | Arm path line 1303 EXISTS. Bar-boundary release MISSING. | GAP — fix required |
| ARP-06 | Arp off resets step counter to 0 | `arpStep_ = 0` line 1334 in off/stopped branch | IMPLEMENTED |

**ARP-03 note:** REQUIREMENTS.md lists "Up, Down, Up-Down, Random" as the four required options. The implementation has 7 options (exceeds spec). The CONTEXT.md explicitly accepts this. No change needed to order patterns.

**ARP-05 note:** This is the only code change required in this phase.

## Open Questions

1. **timeSigNumerator vs. looper_.beatsPerBar() under DAW sync**
   - What we know: When `arpSyncOn` is true, the DAW's reported time signature is the authoritative bar length for the DAW grid. `looper_.beatsPerBar()` reflects the looper's configured subdivison, which may differ (e.g., looper set to 3/4, DAW in 4/4).
   - What's unclear: Whether the user would ever want bar-boundary launch to use looper bar length rather than DAW bar length when sync is ON.
   - Recommendation: Use `timeSigNumer` from DAW when `arpSyncOn` is true (locked by CONTEXT.md). `looper_.beatsPerBar()` is the correct source when `arpSyncOn` is false — but that case is already handled by `looperJustStarted`.

2. **Smoke test scope for ARP-03 (bar-boundary)**
   - What we know: The smoke test checklist in ROADMAP.md success criterion 3 requires: "With DAW sync active, turning the arp on arms it and stepping begins exactly on the next bar boundary — no early or late first note."
   - What's unclear: Whether this can be verified by ear alone or requires a MIDI monitor timestamp comparison.
   - Recommendation: Use the DAW's MIDI monitor (event list view) to compare the timestamp of the first arp note against the bar grid. Ableton's MIDI From monitor shows note-on times in bars/beats.

## Sources

### Primary (HIGH confidence)

- `C:/Users/Milosh Xhavid/get-shit-done/Source/PluginProcessor.cpp` lines 568–582 — position info extraction pattern (verified in source)
- `C:/Users/Milosh Xhavid/get-shit-done/Source/PluginProcessor.cpp` lines 1292–1470 — full arp implementation (verified in source)
- `C:/Users/Milosh Xhavid/get-shit-done/build/_deps/juce-src/modules/juce_audio_basics/audio_play_head/juce_AudioPlayHead.h` lines 351, 500 — `getTimeSignature()` API + `TimeSignature::numerator` field (verified in JUCE 8.0.4 source)
- `C:/Users/Milosh Xhavid/get-shit-done/Source/LooperEngine.cpp` lines 36–48 — `beatsPerBar()` implementation (verified in source)
- `C:/Users/Milosh Xhavid/get-shit-done/.planning/phases/23-arpeggiator/23-CONTEXT.md` — locked implementation decisions

### Secondary (MEDIUM confidence)

- `.planning/REQUIREMENTS.md` lines 87–93 — ARP-01 through ARP-06 canonical requirement text

### Tertiary (LOW confidence)

None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all relevant code verified directly in source files
- Architecture: HIGH — exact insertion points identified with line numbers, JUCE 8 API verified in vendored source
- Pitfalls: HIGH — all pitfalls derived from direct code inspection, not from training assumptions

**Research date:** 2026-03-01
**Valid until:** 2026-04-01 (codebase is stable; JUCE version is locked at 8.0.4 via FetchContent)
