# Architecture Research

**Domain:** JUCE MIDI Plugin (VST3/AUv3)
**Researched:** 2026-02-21
**Confidence:** HIGH

## Standard Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     PluginEditor (GUI)                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ JoystickUI  │  │ Touchplates │  │  Controls   │       │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │
│         │                 │                 │               │
│         └─────────────────┼─────────────────┘               │
│                           │                                 │
│                    Parameter Binding                        │
│                           │                                 │
├───────────────────────────┼─────────────────────────────────┤
│                    PluginProcessor                          │
│  ┌────────────────────────┴────────────────────────┐       │
│  │              AudioProcessorValueTreeState        │       │
│  │  (Parameters: scales, intervals, transpose...)  │       │
│  └────────────────────────┬────────────────────────┘       │
│                           │                                  │
│  ┌─────────────┐  ┌───────┴───────┐  ┌─────────────────┐    │
│  │ MidiInput  │  │  processBlock │  │   MidiOutput   │    │
│  │   Handler  │──│  (MIDI Gen)   │──│    Handler     │    │
│  └─────────────┘  └───────────────┘  └─────────────────┘    │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              Timing/Looper Engine                    │    │
│  │    (Clock sync, subdivisions, loop recording)      │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| **PluginProcessor** | Audio/MIDI processing, parameter management, state | Subclass of `juce::AudioProcessor`, single instance per plugin |
| **PluginEditor** | GUI rendering, user input handling | Subclass of `juce::AudioProcessorEditor`, destroyed/recreated on UI open |
| **APVTS** | Parameter storage, automation, undo/redo | `juce::AudioProcessorValueTreeState` — the central state store |
| **MidiInputHandler** | Receive/parse incoming MIDI | Process in `processBlock()`, use `MidiBuffer` |
| **MidiOutputHandler** | Generate/output MIDI messages | Create `juce::MidiMessage`, add to output `MidiBuffer` |
| **TimingEngine** | DAW sync, clock generation, looper | `juce::AudioProcessor::getSampleRate()`, `juce::Time` |
| **Scale/ChordEngine** | Quantize notes to scales, interval calculation | Separate helper class, lookup tables |
| **LooperEngine** | Record/playback gesture sequences | Ring buffers, transport state machine |

## Recommended Project Structure

```
ChordJoystick/
├── Source/
│   ├── Plugin/
│   │   ├── ChordJoystickProcessor.cpp   # processBlock, APVTS
│   │   └── ChordJoystickProcessor.h
│   ├── Editor/
│   │   ├── ChordJoystickEditor.cpp       # GUI setup, paint, resized
│   │   ├── ChordJoystickEditor.h
│   │   ├── Components/
│   │   │   ├── JoystickComponent.cpp     # XY pad control
│   │   │   ├── TouchplateComponent.cpp    # 4 trigger buttons
│   │   │   ├── ScaleSelector.cpp          # Dropdown for scales
│   │   │   └── KnobComponent.cpp         # Rotary controls
│   │   └── LookAndFeel/
│   │       └── CustomLookAndFeel.cpp
│   ├── Core/
│   │   ├── MidiGenerator.cpp             # 4-note chord generation
│   │   ├── MidiGenerator.h
│   │   ├── ScaleEngine.cpp               # Scale quantization
│   │   ├── ScaleEngine.h
│   │   ├── TimingEngine.cpp               # Clock sync, subdivisions
│   │   └── TimingEngine.h
│   ├── Looper/
│   │   ├── LooperEngine.cpp              # Record/playback logic
│   │   ├── LooperEngine.h
│   │   └── LoopBuffer.cpp
│   ├── Parameters/
│   │   ├── Parameters.cpp                # APVTS setup, parameter IDs
│   │   └── Parameters.h
│   └── Utilities/
│       ├── MidiUtils.cpp                 # MIDI helpers
│       └── Constants.h
├── Resources/
│   └── Info.plist                        # Plugin metadata
├── CMakeLists.txt                        # or .jucer
└── README.md
```

### Structure Rationale

- **Plugin/**: The mandatory `AudioProcessor` subclass — single instance, lives on audio thread
- **Editor/**: GUI components, hierarchy mirrors visual layout
- **Core/**: Music logic (scales, chords, timing) — should be testable independently of JUCE
- **Looper/**: Self-contained looper logic, minimal JUCE dependencies for testability
- **Parameters/**: Single source of truth for all parameter IDs and APVTS layout
- **Utilities/**: Non-domain helpers

## Architectural Patterns

### Pattern 1: Processor-Editor Separation

**What:** Strict separation between audio thread (Processor) and UI thread (Editor). Only APVTS bridges them.

**When to use:** Always. This is the JUCE canonical pattern.

**Trade-offs:**
- Pro: Thread-safe parameter access, automation works automatically
- Pro: Clear ownership — Processor owns state, Editor displays it
- Con: Must not call Processor methods from Editor directly except via APVTS

**Example:**
```cpp
// Processor.h - owns the state
AudioProcessorValueTreeState parameters;

// Editor.cpp - subscribes to parameter changes
parameters.addParameterListener("scale", this);
void parameterChanged(const String&, float newValue) override {
    // Update UI
}
```

### Pattern 2: Real-Time Safe Processing

**What:** No memory allocation in `processBlock()`. Pre-allocate buffers in `prepareToPlay()`.

**When to use:** Any audio/MIDI processing code

**Trade-offs:**
- Pro: Prevents audio glitches, xruns
- Con: Requires upfront design of buffer sizes

**Example:**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    chordBuffer.setSize(4, samplesPerBlock);  // Pre-allocate
}

void processBlock(MidiBuffer& midiMessages, ...) {
    // No new/delete here — use pre-allocated buffers
}
```

### Pattern 3: Parameter-Driven State

**What:** All mutable state flows through APVTS parameters. No ad-hoc member variables for things that need automation.

**When to use:** All plugin parameters

**Trade-offs:**
- Pro: Automatic DAW integration, undo/redo, save/restore
- Con: Slight boilerplate to set up

## Data Flow

### MIDI Output Generation

```
User Input (Joystick/Touchplate)
    ↓
Editor captures input, updates APVTS
    ↓
Processor reads parameter in processBlock()
    ↓
ScaleEngine quantizes to selected scale
    ↓
MidiGenerator creates 4-note chord
    ↓
Output MidiBuffer ← processBlock() returns
```

### Looper Data Flow

```
DAW Transport / Standalone Clock
    ↓
TimingEngine (master/slave mode)
    ↓
LooperEngine.record() captures gesture events
    ↓
LoopBuffer stores (timestamp, event) pairs
    ↓
LooperEngine.playback() loops at subdivision boundaries
    ↓
Events merged into output MidiBuffer
```

### State Synchronization

```
APVTS (single source of truth)
    ↓ (parameterChanged callback)
Editor ← updates display
Processor ← reads in processBlock()
    ↓
getStateInformation() ← serializes for DAW
setStateInformation() ← restores on load
```

## Build Order & Dependencies

### Phase 1: Core Plugin Shell
1. Create Projucer/CMake project with VST3/AUv3 targets
2. Implement minimal `PluginProcessor` + `PluginEditor`
3. Verify plugin loads in DAW

### Phase 2: Parameter Infrastructure
1. Define all parameter IDs in `Parameters.h`
2. Set up APVTS with full layout
3. Add parameterChanged() handling in Editor

### Phase 3: MIDI Generation
1. `ScaleEngine` — scale lookup tables, note quantization
2. `MidiGenerator` — chord construction from joystick position
3. `processBlock()` — wire up MIDI output

### Phase 4: GUI Components
1. Build `JoystickComponent` (XY pad)
2. Build `TouchplateComponent` (4 triggers)
3. Add knobs, selectors
4. Connect to APVTS via `SliderAttachment`/`ComboBoxAttachment`

### Phase 5: Timing & Looper
1. `TimingEngine` — implement DAW sync (hostInfo) vs standalone
2. `LooperEngine` — record/playback with subdivision handling

## Anti-Patterns

### Anti-Pattern 1: Editor Direct Access to Processor State

**What people do:** Editor stores direct pointer to processor's internal variables and reads them directly.

**Why it's wrong:** Race condition — Editor runs on UI thread, Processor on audio thread. DAW automation won't work.

**Do this instead:** Use APVTS as intermediary. Editor reads from APVTS, Processor reads from APVTS.

### Anti-Pattern 2: Memory Allocation in processBlock()

**What people do:** Using `std::vector`, `new`, or `juce::String` inside `processBlock()`.

**Why it's wrong:** Non-deterministic timing causes audio glitches, xruns, and buffer underruns.

**Do this instead:** Pre-allocate everything in `prepareToPlay()`. Use fixed-size buffers, ring buffers, or `alloca()` for small temp data.

### Anti-Pattern 3: Blocking Operations in processBlock()

**What people do:** File I/O, lock(), or heavy computation inside `processBlock()`.

**Why it's wrong:** Missed audio callbacks = dropout.

**Do this instead:** All heavy work on message thread or separate worker thread. Only fast DSP in `processBlock()`.

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| MVP (1 user) | Single processor instance, all in one class is fine |
| 100+ users | Refactor to separate engines (Scale, Midi, Looper) for testability |
| Cross-instance state | Don't use static — each plugin instance is separate process |

### Scaling Priorities

1. **First bottleneck:** GUI responsiveness → move calculation to separate classes
2. **Second bottleneck:** Looper memory → use ring buffers, lazy recording

## Integration Points

### DAW Communication

| DAW Feature | Integration Point | Notes |
|-------------|-------------------|-------|
| Automation | APVTS parameters | Automatic via `RangedAudioParameter` |
| Presets | `getStateInformation()` / `setStateInformation()` | Binary or XML |
| Transport sync | `AudioProcessor::getHostContext()` | Query BPM, time position |
| MIDI I/O | `processBlock(MidiBuffer&, ...)` | Input via MidiBuffer input, output via output param |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Editor ↔ Processor | APVTS only | No direct method calls across threads |
| ScaleEngine ↔ MidiGenerator | Function calls | Both on audio thread, safe |
| TimingEngine ↔ LooperEngine | Shared state | Use `std::atomic` for thread-safe flags |

## Sources

- JUCE Tutorial: Create a basic Audio/MIDI plugin, Part 2 (2025) — `https://juce.com/tutorials/tutorial_code_basic_plugin/`
- JUCE Tutorial: Adding plug-in parameters (2025) — `https://docs.juce.com/master/tutorial_audio_parameter.html`
- JUCE Tutorial: Plugin examples (Arpeggiator, NoiseGate) (2025) — `https://docs.juce.com/master/tutorial_plugin_examples.html`
- JUCE Tutorial: Saving and loading plugin state (2025) — `https://docs.juce.com/master/tutorial_audio_processor_value_tree_state.html`
- Melatonin: Big list of JUCE tips and tricks (2024) — `https://melatonin.dev/blog/big-list-of-juce-tips-and-tricks/`

---
*Architecture research for: JUCE MIDI Plugin*
*Researched: 2026-02-21*
