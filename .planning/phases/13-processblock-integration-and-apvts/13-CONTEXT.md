# Phase 13 Context — processBlock Integration and APVTS

## Phase Boundary

Wire `LfoEngine` into `PluginProcessor`: 14 APVTS parameters, `process()` call in
`processBlock`, LFO output applied in `buildChordParams()`, phase reset on DAW
transport start. **No UI in this phase** — that is Phase 14.

Requirements in scope: LFO-01, LFO-02, LFO-08, LFO-09, LFO-10.

---

## Implementation Decisions

### 1. LFO Mixing Model

**Formula:** `effective_axis = clamp(joystick_pos + lfo_output * level, -1.0f, 1.0f)`

- LFO is an **additive offset**; joystick position is the center of oscillation.
- At joystick = 0 and level = 1.0, the LFO sweeps the full axis range [-1, 1].
- At joystick = 0.5 and level = 1.0, the LFO swings from -0.5 to 1.0 (clamped at 1.0).
- Moving the joystick shifts where the LFO oscillates around — not a multiplicative scale.
- **Level is per-axis**: separate depth parameter for X-LFO (affects fifth/tension voices)
  and Y-LFO (affects root/third voices).
- **At level = 0: LFO is fully bypassed** — no phase accumulation, no CPU cost. Phase
  does NOT advance while bypassed; re-enabling starts from phase 0 (or synced position
  if DAW sync is active).

### 2. Boundary / Clamping Behavior

- **Hard clamp at ±1** — symmetric on both positive and negative edges.
- No soft saturation or wrapping; value hits the wall and stays there until LFO retreats.
- Level range is **0.0 to 2.0** — over-modulation is allowed; clamping keeps output
  in range. This enables aggressive sweeps where the LFO peaks are clipped flat.
- **Disabling the LFO mid-phrase**: output ramps to raw joystick value over ~10 ms
  (linear ramp sufficient). No hard snap; no audible click or pitch jump.

### 3. Sample-and-Hold + Trigger Interaction

- **S&H captures the post-LFO modulated value** at trigger time — not the raw joystick.
  Each trigger fires at whatever `effective_axis` is at that exact moment.
- **After trigger, pitch freezes at the sampled value.** LFO does NOT continue to
  modulate the sustained note. The LFO only influences what pitch is captured.
- This applies uniformly to **all trigger sources**: joystick-trigger, random, and
  touchplate. No special-casing per source.
- Consequence: pad-triggered notes also capture the LFO-modulated position at press time,
  then sustain at that fixed pitch.

### 4. APVTS Parameter Specifications

All 14 parameters are per-axis (X and Y each get their own set):

| Parameter       | Range          | Default   | Notes                                       |
|-----------------|----------------|-----------|---------------------------------------------|
| lfoXEnabled     | bool           | false     | Bypass at false — no phase advance          |
| lfoYEnabled     | bool           | false     | Bypass at false — no phase advance          |
| lfoXWaveform    | 0–6 (enum)     | 0 (Sine)  | Sine/Tri/SawUp/SawDown/Square/S&H/Random   |
| lfoYWaveform    | 0–6 (enum)     | 0 (Sine)  |                                             |
| lfoXRate        | 0.01–20.0 Hz   | 1.0 Hz    | Free mode only; sync uses subdivision enum |
| lfoYRate        | 0.01–20.0 Hz   | 1.0 Hz    |                                             |
| lfoXLevel       | 0.0–2.0        | 0.0       | 0 = no effect; 1.0 = full axis sweep        |
| lfoYLevel       | 0.0–2.0        | 0.0       |                                             |
| lfoXPhase       | 0.0–360.0 deg  | 0.0°      | Displayed in degrees in UI (Phase 14)       |
| lfoYPhase       | 0.0–360.0 deg  | 0.0°      |                                             |
| lfoXDistortion  | 0.0–1.0        | 0.0       | Subtle drive/shaping only — not heavy fold  |
| lfoYDistortion  | 0.0–1.0        | 0.0       |                                             |
| lfoXSync        | bool           | false     | Sync to DAW beat; rate param switches to subdiv |
| lfoYSync        | bool           | false     |                                             |

**Preset compatibility:** New parameters default to `enabled=false, level=0` — loading a
v1.3 preset produces identical behavior to before (no modulation).

---

## Claude's Discretion

- Beat-sync subdivision options (e.g., 1/1, 1/2, 1/4, 1/8, 1/16, dotted, triplet) —
  pick standard musical subdivisions.
- Internal parameter ID naming convention (e.g., `lfoXRate` vs `lfo_x_rate`) — pick
  whatever matches the existing APVTS parameter naming style in PluginProcessor.
- Ramp duration for LFO disable transition: ~10 ms is the target; exact sample count
  is implementation detail.
- Whether `lfoXSync` and `lfoYSync` replace or override the rate parameter — implement
  whichever is cleaner given LfoEngine's existing beat detection API.

---

## Deferred Ideas

*(Mentioned during discussion but out of scope for Phase 13)*

- LFO visualization / waveform preview in UI → **Phase 14**
- Per-voice LFO routing (different LFO settings per voice) → possible future phase
- Stereo LFO (independent L/R phase offset) → not applicable; MIDI effect only
