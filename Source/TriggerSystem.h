#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <functional>

// Per-voice trigger source
enum class TriggerSource { TouchPlate = 0, Joystick = 1, Random = 2 };

// Random subdivision options (beats per bar, denominator)
enum class RandomSubdiv { Quarter = 0, Eighth, Sixteenth, ThirtySecond };

// ─── TriggerSystem ────────────────────────────────────────────────────────────
// Manages gate state for 4 voices.
// Callers feed it events; it produces note-on/note-off callbacks at sample
// accurate positions within the audio block.
//
// Thread safety: UI/gamepad writes go through atomic booleans for the
// physical-button states.  The processBlock call reads those atomics.

class TriggerSystem
{
public:
    // Called when a note-on or note-off must fire.
    // (voiceIndex, midiPitch, isNoteOn, sampleOffset)
    using NoteCallback = std::function<void(int, int, bool, int)>;

    // Called when a raw MIDI message must be emitted (pitch bend, CC, RPN).
    // (midiMessage, sampleOffset)
    using MidiCallback = std::function<void(const juce::MidiMessage&, int)>;

    TriggerSystem();

    // ── Called from UI / gamepad thread ──────────────────────────────────────
    // Physical pad press / release
    void setPadState(int voice, bool pressed);

    // Joystick-movement trigger: call this whenever the joystick moves
    // enough to fire a joystick-sourced voice
    void notifyJoystickMoved();

    // All-notes trigger (gamepad L3)
    void triggerAllNotes();

    // ── Called from audio thread (processBlock) ───────────────────────────────
    struct ProcessParams
    {
        NoteCallback   onNote;
        MidiCallback   onMidi;          // for pitch bend / CC / RPN (may be null)
        int            blockSize        = 512;
        double         sampleRate       = 44100.0;
        double         bpm              = 120.0;

        TriggerSource  sources[4]       = {};
        int            heldPitches[4]   = {};   // current quantized pitch per voice
        int            midiChannels[4]  = {};   // 1-based

        // Random trigger params
        RandomSubdiv   randomSubdiv     = RandomSubdiv::Eighth;
        float          randomDensity    = 0.5f; // 0..1 probability

        // Joystick movement threshold for joystick-source trigger
        float          joystickX        = 0.0f;
        float          joystickY        = 0.0f;
        float          joystickThreshold = 0.015f;  // configurable threshold (0.001..0.1)
    };

    void processBlock(const ProcessParams& p);

    // Returns true if voice gate is currently open
    bool isGateOpen(int voice) const { return gateOpen_[voice].load(); }

    // ── Pitch management ─────────────────────────────────────────────────────
    // The last pitch that was sent as note-on for each voice (for note-off tracking)
    int getActivePitch(int voice) const { return activePitch_[voice]; }

    // Reset all gate state — call from releaseResources() and processBlockBypassed()
    void resetAllGates();

private:
    // Physical pad state (written by UI/gamepad, read by audio thread)
    std::array<std::atomic<bool>, 4> padPressed_   {};
    std::array<std::atomic<bool>, 4> padJustFired_ {};   // edge detection
    std::atomic<bool>                allTrigger_   {false};
    std::atomic<bool>                joystickTrig_ {false};

    // Audio-thread state
    std::array<std::atomic<bool>, 4> gateOpen_     {};
    std::array<int, 4>               activePitch_  {-1,-1,-1,-1};

    // Joystick continuous gate state (pitch bend model)
    std::array<int, 4>           joyBasePitch_         {0, 0, 0, 0};  // MIDI note that was note-on'd when gate opened (bend reference)
    std::array<int, 4>           joyLastBendValue_     {0, 0, 0, 0};  // last pitch bend value sent, to avoid redundant messages
    std::array<int, 4>           joystickStillSamples_ {0, 0, 0, 0};  // counts samples below threshold for 50ms debounce

    // Random trigger clock
    double   randomPhase_     = 0.0;  // samples since last subdivision
    double   prevJoystickX_   = 0.0;
    double   prevJoystickY_   = 0.0;

    // Random number (LCG, audio-thread only)
    uint32_t rng_             = 0x1234ABCD;
    float    nextRandom()     { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 1) / float(0x7FFFFFFF); }

    // Send pitch bend range RPN (±24 semitones) on the given 1-based channel
    void sendBendRangeRPN(int ch1based, int sampleOff, const ProcessParams& p);

    void fireNoteOn (int voice, int pitch, int ch, int sampleOff, const ProcessParams& p);
    void fireNoteOff(int voice, int ch, int sampleOff, const ProcessParams& p);
};
