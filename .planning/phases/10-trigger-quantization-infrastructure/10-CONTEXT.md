# Phase 10: Trigger Quantization Infrastructure - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Recorded gate events in the LooperEngine can be snapped to a user-selected subdivision grid, both live during recording and after recording via a non-destructive toggle. No data races on the audio thread — quantize state is managed with the existing `pendingQuantize_`-style flag pattern.

Technical contract (from success criteria, locked):
- Live quantize: gate events land on nearest grid boundary for the selected subdivision
- Post-record toggle: modifies beatPositions in-memory; shadow copy preserves originals for revert
- QUANTIZE toggle disabled only while **actively recording** (enabled during playback and stopped)
- Wrap edge case: `std::fmod(quantized, loopLen)` required — beatPosition == loopLen wraps to 0.0
- Subdivision selector: 1/4, 1/8, 1/16, 1/32 — operates independently from per-voice random gate subdivision
- Catch2 test required for the wrap edge case before LooperEngine integration

SC3 update from discussion: roadmap says "disabled while playing or recording" — user decision overrides this to **disabled only while recording**. Button is enabled during playback so the user can toggle quantize preview mid-loop.

</domain>

<decisions>
## Implementation Decisions

### Mode selector
- Widget: **3-way segmented button** — `Live | Post | Off`
- Lives **inline with the looper controls** inside the Looper section, adjacent to the single subdivision dropdown
- Default state on fresh load (no APVTS state): **Off**
- APVTS saves and restores the full toggle state (mode + subdivision)

### Subdivision selector
- **One shared dropdown** for both Live mode and Post mode — they use the same grid
- Options: **1/4, 1/8, 1/16, 1/32**
- Stored in APVTS; persists across sessions
- Independent from per-voice random gate subdivision params

### Live quantize behavior
- **Snap timing:** Note-on snaps to nearest grid boundary; note-off = snapped note-on + original duration (gate shifts as a rigid unit — duration preserved exactly)
- **Snap direction:** Nearest grid point; ties (exact midpoint) snap to the **earlier** grid point (prefer the beat already passed, avoid "jumped a beat" feeling)
- **Scope:** Global — applies to all 4 voices simultaneously; no per-voice enable

### Post-record QUANTIZE toggle
- **Mechanism:** Non-destructive toggle (the `Post` position on the mode selector acts as the QUANTIZE toggle)
  - Toggle ON (button/mode lights red): compute quantized beatPositions, overwrite `playbackStore_[]`, save originals to shadow copy
  - Toggle OFF: instant revert — future events in the current loop cycle use original positions; gates already fired this cycle are not affected
- **Availability:** Enabled when **not recording** (playback OK, stopped OK); disabled while record is active
- **Feedback:** Mode button segment highlighted red when Post mode is active (toggle on)
- **New recording while Post is ON:** New record overwrites both originals and the shadow copy; Post mode remains active but now quantizes the new content

### Gate duration edge cases
- **Negative gate:** If quantized note-on ≥ original note-off, enforce **1/64-note minimum gate duration** floor (note-off = snapped note-on + 1/64 note)
- **Collision (same beat position, same voice):** Keep the first event, discard the second
- **Post-record:** Same rules as live — note-on snap + clamped duration (not a lighter touch)

### APVTS parameters to add
- `quantizeMode` — int/enum: 0=Off, 1=Live, 2=Post
- `quantizeSubdiv` — int: 0=1/4, 1=1/8, 2=1/16, 3=1/32

</decisions>

<specifics>
## Specific Ideas

- The toggle visual feedback is "lights up red" — consistent with the user's language throughout the project for active/danger states
- The wrap fmod guard (`std::fmod(quantized, loopLen)`) is a known requirement logged in STATE.md — must be covered by Catch2 test before LooperEngine integration
- Shadow copy for non-destructive revert lives alongside `playbackStore_[]` — planner should decide exact field names; `originalStore_[]` is a reasonable candidate
- Snap direction (ties go earlier) matches musical intuition: a gate fired exactly halfway between beats 1 and 2 snaps back to beat 1, not forward to beat 2

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 10-trigger-quantization-infrastructure*
*Context gathered: 2026-02-25*
