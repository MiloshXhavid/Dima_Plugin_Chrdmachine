# Phase 15 Context — Gamepad Preset Control

## Phase Boundary

Add an Option-button toggle that remaps the BPM±1 gamepad controls to MIDI Program
Change +1/−1. Show a color-changed `"OPTION"` label in the gamepad controls section of
the UI when the mode is active. **No new MIDI channel parameter** — Program Changes use
the existing `filterMidiCh` APVTS parameter.

Requirements in scope: CTRL-01, CTRL-02, CTRL-03.

---

## Implementation Decisions

### 1. Mode Toggle Behavior

- **Toggle on press**: Option button activates preset-scroll mode on the first press,
  deactivates on the next press. State flips at each SDL button-down event.
- **Always starts deactivated**: `presetScrollActive = false` on plugin load. Mode state
  is NOT persisted in plugin saved state.
- **Each BPM+/− press → exactly one Program Change**: No rate-repeat while held. The
  Program Change fires once per button release (button-up event).
- **In-flight BPM completes first**: If a BPM increment is already in progress when the
  Option toggle fires, the BPM change completes, then the mode switches.
- **Gamepad only**: No APVTS parameter exposed for external toggling. The Option button
  on the PS/Xbox controller is the sole activation mechanism.

### 2. Program Change Routing

- **MIDI channel**: Reuse the existing `filterMidiCh` APVTS parameter. No new channel
  parameter or UI control added.
- **Internal counter**: Tracks current program number (0–127) for generating absolute
  PC messages. Initializes to 0 on plugin load. Not persisted in saved state.
- **Send timing**: Program Change fires on **button release** (SDL button-up), not
  button-down.
- **Boundary behavior**: **Clamp at 0 and 127**. Pressing BPM+ at program 127 does
  nothing; pressing BPM− at program 0 does nothing. No wrap-around.
- **No UI display of program number**: The indicator only shows that the mode is active.
  No numeric counter shown in the UI.

### 3. UI Indicator Design

- **Location**: Gamepad controls section of the plugin UI (wherever the existing gamepad
  status area is — the section that shows connection state, etc.).
- **Visual**: Color change + label `"OPTION"` on the Option button representation.
  The color change signals mode-active; the label identifies which mode.
- **Prominence**: Subtle — secondary visual weight. Not blinking or animating.
  Visible if looked at; not distracting during normal play.
- **Gamepad disconnect while active**: Mode stays active (UI `presetScrollActive` flag
  unchanged). The indicator reflects the disconnected gamepad state (existing disconnect
  styling), but the `"OPTION"` color/label persists to signal the mode is still on.

### 4. BPM Control Lock-out

- **BPM value unaffected**: Pressing Option does NOT freeze the BPM. The plugin UI BPM
  knob, DAW automation, and all non-gamepad paths continue to work normally.
- **Only the gamepad button mapping changes**: In preset-scroll mode, the BPM+/− gamepad
  buttons send Program Change instead of incrementing/decrementing BPM. Everything else
  (looper, voices, reset, record, etc.) is unchanged.
- **One-frame lockout**: If the Option button fires in a polling frame, BPM+/− button
  inputs in that same frame are ignored. Prevents accidental Program Change or BPM change
  on the transition frame.

---

## Claude's Discretion

- SDL polling wiring: integrate into the existing `GamepadInput` timer callback; pick
  the cleanest pattern matching what's already there for the looper/voice buttons.
- Where exactly `presetScrollActive` state lives: `GamepadInput` member vs. passed into
  `PluginProcessor` — whichever is cleaner given the existing architecture.
- Exact color token for the `"OPTION"` active state: match the existing active/highlight
  color convention used elsewhere in the UI.
- Whether the internal program counter lives in `GamepadInput` or `PluginProcessor` —
  PC messages need to go through `PluginProcessor`'s MIDI output, so the counter likely
  belongs there.

---

## Deferred Ideas

*(Mentioned during discussion but out of scope for Phase 15)*

- Dedicated Program Change MIDI channel selector (separate from filterMidiCh) → possible
  future phase if the user's setup requires PC and filter CC on different channels
- Displaying current program number in the UI → out of scope
