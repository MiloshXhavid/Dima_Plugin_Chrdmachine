#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <functional>

// Per-voice trigger source
enum class TriggerSource { TouchPlate = 0, Joystick = 1, RandomFree = 2, RandomHold = 3 };

// Random subdivision options (beats per bar, denominator)
enum class RandomSubdiv { QuadWhole = 0, DblWhole = 1, Whole = 2, Half = 3, Quarter = 4, Eighth = 5, Sixteenth = 6, ThirtySecond = 7, SixtyFourth = 8 };

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
    using NoteCallback      = std::function<void(int, int, bool, int)>;

    // Called when pitch bend must be emitted for a JOY-mode voice.
    // (voiceIndex, bipolar14bit [-8192..+8191], channel0based, sampleOffset)
    // +8191 = full up (bend range semitones), 0 = centre, -8192 = full down.
    using PitchBendCallback = std::function<void(int, int, int, int)>;

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
        NoteCallback        onNote;
        PitchBendCallback   onPitchBend;   // optional — null = no pitch bend
        int            blockSize        = 512;
        double         sampleRate       = 44100.0;
        double         bpm              = 120.0;

        TriggerSource  sources[4]       = {};
        int            heldPitches[4]   = {};   // current quantized pitch per voice
        int            midiChannels[4]  = {};   // 1-based

        // Random trigger params (per-voice)
        RandomSubdiv   randomSubdiv[4]  = {};          // per-voice subdivision (was single value)
        float          randomPopulation  = 4.0f;        // max gates fired per bar (1–64)
        float          randomProbability = 1.0f;        // per-slot fire chance (0.0–1.0); 1.0 = always fire
        float          gateLength        = 0.5f;        // note hold fraction (0.0=manual open gate, 0.0–1.0)

        // Timing (for DAW-synced random clock)
        double         ppqPosition       = -1.0;        // beats since session start; -1 = not available
        bool           isDawPlaying      = false;

        // Joystick absolute position (used by ChordEngine for pitch — kept for reference)
        float          joystickX        = 0.0f;
        float          joystickY        = 0.0f;

        // Joystick delta since last block (used by JOY gate detection)
        float          deltaX           = 0.0f;
        float          deltaY           = 0.0f;

        float          joystickThreshold = 0.015f;  // movement sensitivity (0.001..0.1)
        float          joystickGateTime  = 1.0f;   // seconds of stillness before gate closes

        // Random clock mode
        bool           randomClockSync  = false;   // true = DAW-gated, false = free tempo
        float          randomFreeTempo  = 120.0f;  // BPM for free mode (30..240)
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

    // Joystick continuous gate state
    std::array<int, 4>           joyActivePitch_       {-1,-1,-1,-1}; // pitch currently sounding per voice, -1 = none
    std::array<int, 4>           joystickStillSamples_ {0, 0, 0, 0};  // samples accumulated without movement (gate-close countdown)
    std::array<int, 4>           joyOpenPitch_         {-1,-1,-1,-1}; // pending pitch (reset on each pitch change, fires on settle)
    std::array<int, 4>           joySettleSamples_     {0, 0, 0, 0};  // samples remaining in settle debounce (note fires at 0)
    std::array<int, 4>           joyLastBend_          {0, 0, 0, 0};  // last sent pitch bend value (dedup)

    // Random trigger clock (per-voice)
    std::array<double,   4> randomPhase_         {};           // samples since last subdiv (fallback clock)
    std::array<int64_t,  4> prevSubdivIndex_     {-1,-1,-1,-1}; // ppq subdivision index last seen
    std::array<int,      4> randomGateRemaining_ {};           // samples until auto note-off
    bool                    wasPlaying_          = false;      // for transport restart detection
    std::array<TriggerSource, 4> prevSrc_        {};           // previous source per voice (mode-switch transition detection)

    double   prevJoystickX_   = 0.0;
    double   prevJoystickY_   = 0.0;

    // Random number (LCG, audio-thread only)
    uint32_t rng_             = 0x1234ABCD;
    float    nextRandom()     { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 1) / float(0x7FFFFFFF); }

    void fireNoteOn (int voice, int pitch, int ch, int sampleOff, const ProcessParams& p);
    void fireNoteOff(int voice, int ch, int sampleOff, const ProcessParams& p);
};
