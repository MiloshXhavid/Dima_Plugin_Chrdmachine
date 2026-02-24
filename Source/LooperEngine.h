#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>

// ─── Debug thread-safety assertion ───────────────────────────────────────────
// ASSERT_AUDIO_THREAD() fires in Debug builds if called from the message thread.
// Use at the top of every audio-thread-only method to catch misuse early.
#if JUCE_DEBUG
  #define ASSERT_AUDIO_THREAD() jassert(!juce::MessageManager::existsAndIsCurrentThread())
#else
  #define ASSERT_AUDIO_THREAD() ((void)0)
#endif

// Time signature / subdivision for loop length
enum class LooperSubdiv
{
    ThreeFour   = 0,  // 3/4  — 3.0 beats/bar
    FourFour    = 1,  // 4/4  — 4.0 beats/bar
    FiveFour    = 2,  // 5/4  — 5.0 beats/bar
    SevenEight  = 3,  // 7/8  — 3.5 beats/bar
    NineEight   = 4,  // 9/8  — 4.5 beats/bar
    ElevenEight = 5,  // 11/8 — 5.5 beats/bar
};

// ─── LooperEngine ─────────────────────────────────────────────────────────────
// Records timestamped joystick + gate events and plays them back looped,
// synced to the DAW playhead (or an internal counter if no DAW sync).
//
// Timeline unit: beats (quarter-note = 1.0)
//
// LOCK-FREE DESIGN: No std::mutex anywhere in the audio-thread call path.
// Double-buffer model:
//   - eventBuf_[] + fifo_       : SPSC FIFO — audio thread writes during record
//   - playbackStore_[]          : stable read buffer during playback
//   - finaliseRecording()       : copies FIFO → playbackStore_ when recording stops
//                                 (called only when playing_=false AND recording_=false,
//                                  so no concurrent FIFO reader/writer exists)

class LooperEngine
{
public:
    // ── Capacity ──────────────────────────────────────────────────────────────
    static constexpr int LOOPER_FIFO_CAPACITY = 2048;

    // ── Event type ────────────────────────────────────────────────────────────
    struct LooperEvent
    {
        double beatPosition;   // beat offset within loop (0..loopLengthBeats)
        enum class Type { JoystickX, JoystickY, FilterX, FilterY, Gate } type;
        int    voice;          // gate events: which voice (0-3); joystick: -1
        float  value;          // joystick: -1..1; gate: 1=on, 0=off
    };

    // ── Process I/O ───────────────────────────────────────────────────────────
    struct ProcessParams
    {
        double sampleRate;
        double bpm;
        double ppqPosition;    // from AudioPlayHead (or -1 if no DAW sync)
        int    blockSize;
        bool   isDawPlaying;
    };

    struct BlockOutput
    {
        bool    hasJoystickX = false;
        float   joystickX    = 0.0f;
        bool    hasJoystickY = false;
        float   joystickY    = 0.0f;
        bool    hasFilterX   = false;
        float   filterX      = 0.0f;
        bool    hasFilterY   = false;
        float   filterY      = 0.0f;
        bool    gateOn[4]    = {};
        bool    gateOff[4]   = {};
        bool    dawStopped   = false;  // true on the block where DAW stops (send all-notes-off)
    };

    LooperEngine() = default;

    // ── Transport (called from UI / gamepad thread) ───────────────────────────
    void startStop();   // toggle play; on stop: calls finaliseRecording()
    void record();      // arm/disarm recording (only while playing); on disarm: finaliseRecording()
    void reset();       // sets resetRequest_ — audio thread clears state at next process()
    void deleteLoop();  // sets deleteRequest_ — audio thread clears events at next process()

    bool isPlaying()   const { return playing_.load();   }
    bool isRecording() const { return recording_.load(); }
    bool hasContent()  const;

    // ── Configuration (thread-safe — atomic stores) ───────────────────────────
    void setSubdiv(LooperSubdiv s)  { subdiv_.store((int)s); }
    void setLoopLengthBars(int bars);  // clamped 1..16

    // New mode setters (called from PluginEditor or PluginProcessor)
    void setRecJoy(bool b)    { recJoy_.store(b);    }
    void setRecGates(bool b)  { recGates_.store(b);  }
    void setRecFilter(bool b) { recFilter_.store(b); }
    void setSyncToDaw(bool b) { syncToDaw_.store(b); }

    bool isCapReached()  const { return capReached_.load();                                                    }
    bool isRecordArmed() const { return recording_.load() || recordPending_.load() || recWaitArmed_.load(); }
    bool isSyncToDaw()   const { return syncToDaw_.load();                                                  }
    bool isRecJoy()      const { return recJoy_.load();                                                     }
    bool isRecGates()    const { return recGates_.load();                                                   }
    bool isRecFilter()   const { return recFilter_.load();                                                  }

    void setRecWaitForTrigger(bool b) { recWaitForTrigger_.store(b); }
    bool isRecWaitForTrigger()  const { return recWaitForTrigger_.load(); }
    bool isRecWaitArmed()       const { return recWaitArmed_.load(); }

    // Called from audio thread when a note-on fires and recWaitArmed_ is set.
    void activateRecordingNow();

    // ── Audio-thread interface ─────────────────────────────────────────────────
    // All three methods must only be called from the audio thread.
    BlockOutput process(const ProcessParams& p);
    void recordJoystick(double beatPos, float x, float y);
    void recordFilter(double beatPos, float x, float y);
    void recordGate(double beatPos, int voice, bool on);

    // UI read-out
    double getPlaybackBeat() const { return static_cast<double>(playbackBeat_.load()); }
    double getLoopLengthBeats() const;

private:
    // ── FIFO backing store (record side — SPSC, audio thread writes during record) ──
    std::array<LooperEvent, LOOPER_FIFO_CAPACITY> eventBuf_ {};
    juce::AbstractFifo fifo_ { LOOPER_FIFO_CAPACITY };

    // ── Playback stable buffer (populated by finaliseRecording()) ─────────────
    std::array<LooperEvent, LOOPER_FIFO_CAPACITY> playbackStore_ {};
    std::atomic<int> playbackCount_ { 0 };

    // ── Scratch buffers for finaliseRecording() punch-in merge ────────────────
    // Kept as class members to avoid large (~49 KB each) stack allocations in
    // finaliseRecording(), which would risk stack overflow on MSVC debug builds.
    // These are only used inside finaliseRecording() (called with playing_=false
    // AND recording_=false, so no reentrancy risk).
    std::array<LooperEvent, LOOPER_FIFO_CAPACITY> scratchNew_    {};
    std::array<LooperEvent, LOOPER_FIFO_CAPACITY> scratchMerged_ {};

    // ── Atomic state flags (audio + UI threads) ───────────────────────────────
    std::atomic<bool> playing_       { false };
    std::atomic<bool> recording_     { false };
    std::atomic<bool> recordPending_ { false }; // REC pressed, waiting for next valid clock
    std::atomic<bool> recJoy_            { false };  // [REC JOY] armed
    std::atomic<bool> recGates_          { false };  // [REC GATES] armed
    std::atomic<bool> recFilter_         { true };   // filter recording always on
    std::atomic<bool> syncToDaw_         { false };  // [DAW] sync to DAW playhead
    std::atomic<bool> capReached_        { false };  // overflow indicator for UI
    std::atomic<bool> recWaitForTrigger_ { false };  // mode: wait for trigger to start rec
    std::atomic<bool> recWaitArmed_      { false };  // waiting for first note-on

    // ── Destructive op request flags (UI sets, audio thread executes) ─────────
    std::atomic<bool> deleteRequest_ { false };
    std::atomic<bool> resetRequest_  { false };

    // ── Audio-thread-only (no atomic needed) ─────────────────────────────────
    double internalBeat_    = 0.0;
    double loopStartPpq_    = -1.0;  // DAW sync anchor; -1 = not anchored
    float  lastRecordedX_       = 0.0f;  // for sparse pitch joystick recording threshold
    float  lastRecordedY_       = 0.0f;
    float  lastRecordedFilterX_ = 0.0f;  // for sparse filter joystick recording threshold
    float  lastRecordedFilterY_ = 0.0f;
    bool   prevDawPlaying_  = false; // tracks DAW play/stop transitions for auto-stop/start
    double recordedBeats_   = 0.0;   // beats recorded in current pass (auto-stop)

    // ── UI read-out — float atomic (lock-free on all targets, incl. 32-bit) ───
    std::atomic<float> playbackBeat_ { 0.0f };

    // ── Config (atomic for UI thread writes) ──────────────────────────────────
    std::atomic<int>   subdiv_          { (int)LooperSubdiv::FourFour };
    std::atomic<int>   loopBars_        { 2 };
    std::atomic<float> joystickThresh_  { 0.02f };  // sparse recording threshold

    // ── Private helpers ───────────────────────────────────────────────────────
    double beatsPerBar() const;
    void   anchorToBar(double ppqPosition, double bpb);

    // finaliseRecording(): merges FIFO events into playbackStore_ with punch-in logic.
    // THREADING INVARIANT: MUST be called only when playing_=false AND recording_=false.
    // Both callers (startStop() and record()) set these flags before calling this method.
    void finaliseRecording();
};

// Compile-time note: std::atomic<float> is_always_lock_free is true on all
// modern x86/ARM targets. We use float (not double) for playbackBeat_ to ensure
// lock-free guarantee on 32-bit builds where double atomics may not be lock-free.
static_assert(sizeof(float) == 4, "float must be 4 bytes for atomic lock-free guarantee");
