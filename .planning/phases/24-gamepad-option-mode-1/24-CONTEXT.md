# Phase 24: Gamepad Option Mode 1 — Context

**Gathered:** 2026-03-01
**Status:** Ready for planning
**Source:** Codebase scout + discuss-phase session

<domain>
## Phase Boundary

Add face-button arp/random dispatch to Option Mode 1 on the PS gamepad, so players can toggle and adjust the arpeggiator one-handed during live performance. R3 alone is already a no-op (Phase 19). Sub Oct hold+R3 is already implemented (Phase 19). This phase wires Mode 1 face buttons → arp controls and updates the UI mode label.

**What is NOT in scope:** adding new modes, changing D-pad behavior in any mode, changing looper face-button behavior in Mode 0.

</domain>

<decisions>
## Implementation Decisions

### Mode 1 architecture — face buttons + D-pad coexist
Mode 1 = face buttons do ARP dispatch AND D-pad still does octave control (existing behavior). Both work simultaneously because they use different physical buttons. No D-pad behavior changes. The user confirmed: "both at the same time since different buttons."

**Mode 1 face-button mapping (new):**
- Circle → toggle arp on/off
- Triangle (1× press) → step Arp Rate UP one step (1/4 → 1/8T → 1/8 → 1/16T → 1/16 → 1/32 → wraps to 1/4)
- Triangle (2× press within 300ms) → step Arp Rate DOWN one step
- Square (1× press) → step Arp Order UP one step (cycles through all 7 orders)
- Square (2× press within 300ms) → step Arp Order DOWN one step
- X → toggle RND Sync (randomClockSync APVTS param)

**Mode 1 D-pad mapping (unchanged from existing):**
- D-pad Up → rootOctave step
- D-pad Down → thirdOctave step
- D-pad Left → tensionOctave step
- D-pad Right → fifthOctave step

### Mode 2 — unchanged
Mode 2 = INTRVL (D-pad → globalTranspose / thirdInterval / fifthInterval / tensionInterval). The roadmap's mention of "Program Change scrolling" for Mode 2 is outdated — PC scroll was replaced by INTRVL. Phase 24 does NOT touch Mode 2.

### Circle toggle edge cases
- **Looper STOPPED:** toggle arp on → looper reset to beat 0 → start looper playback
- **Looper ALREADY PLAYING:** toggle arp on only — no looper reset, no position jump
- **DAW sync active (any looper state):** toggle arp on → arm arpWaitingForPlay_ (waits for bar boundary per Phase 23 fix)
- **Toggling arp OFF (any state):** just turn arp off — no looper side-effects

### Double-press timing window
Use 300ms — identical to existing `kDpadDoubleClickMs` constant in GamepadInput.h. Reuse the same double-click detection architecture already used for D-pad deltas. Single-press fires direction +1; double-press within 300ms fires direction -1. Apply same `pendingOptionDpadDelta_` pattern but for face buttons (new atomic signals needed).

### R3 behavior
R3 alone = already a no-op (Phase 19 removed R3 panic). Holding pad + R3 = Sub Oct toggle (Phase 19). Phase 24 must NOT change R3 behavior — verify only.

### UI label for Mode 1
Change label from `"OCTAVE"` to `"ARP"` (colour stays green). The arp control is now the primary Mode 1 identity. The D-pad octave functionality remains but is secondary. Label is updated in `timerCallback()` at the same site as Mode 1 / Mode 2 label dispatch in PluginEditor.cpp.

### Claude's Discretion
- Exact names for the new face-button delta atomic signals in GamepadInput.h (e.g., `pendingArpFaceBtn_[4]` or individual atomics per button)
- Whether to reuse the existing `ButtonState` + `debounce()` architecture for face-button double-press or inline the 300ms check
- Where exactly in processBlock to dispatch face-button arp signals (alongside or after the existing Mode 1 D-pad dispatch block)
- APVTS step-clamp bounds for arpSubdiv (0–5) and arpOrder (0–6)

</decisions>

<specifics>
## Specific Implementation Notes

**Existing dispatch architecture (PluginProcessor.cpp ~line 528):**
```cpp
const int optMode = gamepad_.getOptionMode();
if (optMode == 1)
{
    // D-pad octave dispatch (unchanged)
    { const int d = gamepad_.consumeOptionDpadDelta(0); if (d) stepClampingParam(apvts, "rootOctave", ...); }
    // ... 3 more D-pad directions

    // NEW: face-button arp dispatch (add here)
    // e.g. if (gamepad_.consumeArpCircle()) { toggle arpEnabled }
    //      if (int d = gamepad_.consumeArpRateDelta()) { step arpSubdiv }
    //      if (int d = gamepad_.consumeArpOrderDelta()) { step arpOrder }
    //      if (gamepad_.consumeRndSyncToggle()) { flip randomClockSync }
}
else if (optMode == 2) { /* INTRVL — unchanged */ }
```

**Rate wrapping:** arpSubdiv has 6 options (indices 0–5). Step up from 5 wraps to 0; step down from 0 wraps to 5. Use `stepWrappingParam()` (already exists for globalTranspose).

**Order wrapping:** arpOrder has 7 options (indices 0–6). Same wrapping pattern.

**randomClockSync is a bool param.** Toggle = read current value, write !current. No wrapping needed.

**Circle + looper start:** The looper start sequence in processBlock is already handled by `pendingLooperStartStop_`. For the Circle toggle-on-while-stopped case, fire pendingLooperReset_ then pendingLooperStartStop_ (or equivalent atomic sequence that resets to beat 0 then starts). Verify this doesn't double-trigger if looper is already playing.

**UI timerCallback label site (PluginEditor.cpp ~line 2889):**
```cpp
const juce::String newText = (optMode == 0) ? "OPTION"
                           : (optMode == 1) ? "OCTAVE"   // ← change to "ARP"
                           :                  "INTRVL";
```

**Requirements to cover:** OPT1-01, OPT1-02, OPT1-03, OPT1-04, OPT1-05, OPT1-06, OPT1-07
Researcher must read REQUIREMENTS.md to understand OPT1-06 and OPT1-07 (not yet reviewed).

</specifics>

<deferred>
## Deferred Ideas

- PC scroll / Program Change mode on D-pad (was Mode 2 in Phase 15, replaced by INTRVL — if user wants it back, that's a separate phase)
- Mode 3 for future expansion
- Face button behavior in Mode 2 (currently unused in Mode 2)

</deferred>

---

*Phase: 24-gamepad-option-mode-1*
*Context gathered: 2026-03-01 via codebase scout + discuss-phase session*
