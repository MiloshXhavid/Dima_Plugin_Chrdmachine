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
        bool    looperReset  = false;  // true on the block where reset fires (seek to 0, cut looper notes)
        bool    looperCycled = false;  // true on the block where playback position wraps to 0 (not during recording)
    };

    LooperEngine() = default;

    // ── Transport (called from UI / gamepad thread) ───────────────────────────
    void startStop();   // toggle play; on stop: calls finaliseRecording()
    void record();      // arm/disarm recording (only while playing); on disarm: finaliseRecording()
    void reset();       // sets resetRequest_ — audio thread clears state at next process()
    void deleteLoop();  // sets deleteRequest_ — audio thread clears events at next process()

    // Lane-clear requests — called from message thread; serviced by audio thread at top of process().
    // Each flag removes all events of the matching type(s) from playbackStore_.
    void clearGateLane()   { pendingClearGates_.store(true,  std::memory_order_release); }
    void clearJoyLane()    { pendingClearJoy_.store(true,    std::memory_order_release); }
    void clearFilterLane() { pendingClearFilter_.store(true, std::memory_order_release); }

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

    // Quantize config setters (called from PluginProcessor, thread-safe via atomics)
    void setQuantizeMode(int mode)     { quantizeMode_.store(mode,   std::memory_order_relaxed); }
    void setQuantizeSubdiv(int subdiv) { quantizeSubdiv_.store(subdiv, std::memory_order_relaxed); }
    bool isQuantizeActive() const      { return quantizeActive_.load(std::memory_order_relaxed); }

    // Post-record quantize trigger / revert (called from message thread — sets flags only)
    void applyQuantize()  { pendingQuantize_.store(true,       std::memory_order_release); }
    void revertQuantize() { pendingQuantizeRevert_.store(true, std::memory_order_release); }

    bool isCapReached()      const { return capReached_.load();                                                    }
    bool isRecordArmed()     const { return recording_.load() || recordPending_.load() || recWaitArmed_.load(); }
    bool isSyncToDaw()       const { return syncToDaw_.load();                                                  }
    bool isRecJoy()          const { return recJoy_.load();                                                     }
    bool isRecGates()        const { return recGates_.load();                                                   }
    bool isRecFilter()       const { return recFilter_.load();                                                  }
    // True once filter/joystick/gate events exist in playbackStore_ (cleared on delete).
    // Used by processor to allow live stick when nothing has been recorded yet.
    bool hasFilterContent()   const { return hasFilterContent_.load(std::memory_order_relaxed);   }
    bool hasJoystickContent() const { return hasJoystickContent_.load(std::memory_order_relaxed); }
    bool hasGateContent()     const { return hasGateContent_.load(std::memory_order_relaxed);     }

    void setRecWaitForTrigger(bool b) { recWaitForTrigger_.store(b); }
    bool isRecWaitForTrigger()    const { return recWaitForTrigger_.load(); }
    bool isRecWaitArmed()         const { return recWaitArmed_.load(); }
    bool isRecordPending()        const { return recordPending_.load(); }
    bool isRecPendingNextCycle()  const { return recPendingNextCycle_.load(); }

    // Arm "start rec by touch" independently of the REC button.
    // Toggles: first call arms the wait, second call cancels it.
    void armWait();

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
    std::atomic<bool> recJoy_            { true  };  // [REC JOY] armed (on by default)
    std::atomic<bool> recGates_          { true  };  // [REC GATES] armed (on by default)
    std::atomic<bool> recFilter_         { false };  // filter recording off by default (live use)
    std::atomic<bool> syncToDaw_         { false };  // [DAW] sync to DAW playhead
    std::atomic<bool> capReached_         { false };  // overflow indicator for UI
    std::atomic<bool> hasFilterContent_  { false };  // set after finalise if FilterX/Y events exist
    std::atomic<bool> hasJoystickContent_{ false };  // set after finalise if JoystickX/Y events exist
    std::atomic<bool> recWaitForTrigger_    { false };  // mode: wait for trigger to start rec
    std::atomic<bool> recWaitArmed_        { false };  // waiting for first note-on
    std::atomic<bool> recPendingNextCycle_ { false };  // rec armed, starts at next loop boundary

    // ── Destructive op request flags (UI sets, audio thread executes) ─────────
    std::atomic<bool> deleteRequest_ { false };
    std::atomic<bool> resetRequest_  { false };

    // ── Lane-clear pending flags (UI sets, audio thread services at top of process()) ─
    std::atomic<bool> pendingClearGates_  { false };
    std::atomic<bool> pendingClearJoy_    { false };
    std::atomic<bool> pendingClearFilter_ { false };
    std::atomic<bool> hasGateContent_     { false };

    // ── Quantize state ──────────────────────────────────────────────────────────
    // Shadow copy for non-destructive post-record revert
    std::array<LooperEvent, LOOPER_FIFO_CAPACITY> originalStore_ {};
    std::atomic<int>  originalCount_  { 0 };
    std::atomic<bool> hasOriginals_   { false };
    std::atomic<bool> quantizeActive_ { false }; // true = post-quantize currently applied

    // Pending flags: message thread sets, audio thread services at top of process()
    std::atomic<bool> pendingQuantize_       { false };
    std::atomic<bool> pendingQuantizeRevert_ { false };

    // Config: read on audio thread, written from PluginProcessor (APVTS-driven)
    std::atomic<int> quantizeMode_   { 0 };  // 0=Off, 1=Live, 2=Post
    std::atomic<int> quantizeSubdiv_ { 1 };  // 0=1/1T, 1=1/2T, 2=1/4, 3=1/4T, 4=1/8, 5=1/8T, 6=1/16, 7=1/16T, 8=1/32, 9=1/32T

    // Per-voice last note-on beats (audio-thread-only — no atomic needed)
    double lastSnappedOnBeat_[4] = { 0.0, 0.0, 0.0, 0.0 };
    double lastRawOnBeat_[4]     = { 0.0, 0.0, 0.0, 0.0 };  // raw (pre-snap) note-on beat for duration

    // Scratch buffer for applyQuantizeToStore() dedup pass — avoids ~49KB stack alloc
    std::array<LooperEvent, LOOPER_FIFO_CAPACITY> scratchDedup_ {};

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

    // Applies snapToGrid to all Gate-type events in playbackStore_[].
    // Deduplicates collisions (same voice, same snapped beat -> keep first).
    // Re-sorts playbackStore_ by beatPosition after snap.
    // MUST be called only from audio thread (inside process()).
    void applyQuantizeToStore(double gridSize, double loopLen);
};

// Compile-time note: std::atomic<float> is_always_lock_free is true on all
// modern x86/ARM targets. We use float (not double) for playbackBeat_ to ensure
// lock-free guarantee on 32-bit builds where double atomics may not be lock-free.
static_assert(sizeof(float) == 4, "float must be 4 bytes for atomic lock-free guarantee");

// ─── Quantize math utilities (declared here for testability) ──────────────────
// Returns the nearest grid boundary to beatPos within a loop of length loopLen.
// Ties (beatPos exactly at grid midpoint) snap to the EARLIER grid point.
// Result is always in [0, loopLen) via std::fmod.
double snapToGrid(double beatPos, double gridSize, double loopLen) noexcept;

// Converts quantizeSubdiv APVTS index (0..9) to beat-unit grid size.
// 0=1/4(1.0), 1=1/8(0.5), 2=1/16(0.25), 3=1/32(0.125), 4=1/1T(8/3), 5=1/2T(4/3), 6=1/4T(2/3), 7=1/8T(1/3), 8=1/16T(1/6), 9=1/32T(1/12). Default fallback: 0.5.
double quantizeSubdivToGridSize(int subdivIdx) noexcept;
