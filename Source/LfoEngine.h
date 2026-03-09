#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>

// ─── LfoEngine ────────────────────────────────────────────────────────────────
// Self-contained, audio-thread-safe LFO DSP class.
// No APVTS, no LooperEngine, no JUCE-specific includes required.
// Phase 13 wires this into processBlock(); Phase 12 builds it in isolation.
//
// Thread safety: process() only reads its ProcessParams argument and mutates
// private state (phase_, rng_, etc.). It never writes to shared atomics.
// ─────────────────────────────────────────────────────────────────────────────

// ─── Waveform enum ───────────────────────────────────────────────────────────
// Exactly 7 values — order matches APVTS integer parameter mapping.
enum class Waveform { Sine = 0, Triangle, SawUp, SawDown, Square, SH, Random };

// ─── LfoRecState enum ────────────────────────────────────────────────────────
// Recording state machine for LFO capture / playback.
// Stored as std::atomic<int> inside LfoEngine (not std::atomic<LfoRecState>)
// to avoid MSVC C2338 on enum class in atomics.
enum class LfoRecState { Unarmed = 0, Armed, Recording, Playback };

// ─── ProcessParams struct ─────────────────────────────────────────────────────
// All fields with sensible defaults so callers only need to fill what differs.
struct ProcessParams
{
    double   sampleRate    = 44100.0;
    int      blockSize     = 512;
    double   bpm           = 120.0;        // effective BPM (free or DAW)

    // Sync mode
    bool     syncMode      = false;        // false = free Hz, true = beat subdivisions
    double   ppqPosition   = -1.0;        // -1 = not available / DAW not playing
    bool     isDawPlaying  = false;

    // Free mode
    float    rateHz        = 1.0f;        // 0.01..20.0 Hz (log scale)

    // Sync mode — subdivision in beats (quarter-note = 1.0 beat)
    // e.g. 1/8=0.5, 1/4=1.0, dotted 1/4=1.5, triplet 1/4=0.6667
    double   subdivBeats   = 1.0;

    // Maximum slow subdivision: loopLengthBars x beatsPerBar, passed from PluginProcessor
    double   maxCycleBeats = 16.0;

    // Waveform parameters
    Waveform waveform      = Waveform::Sine;
    float    phaseShift    = 0.0f;         // 0.0..1.0 maps to 0..360 degrees
    float    distortion    = 0.0f;         // 0.0..1.0, additive LCG noise post-waveform
    float    level         = 1.0f;         // 0.0..1.0 amplitude / depth
    float    playbackPhase = 0.0f;         // normalized looper position [0.0, 1.0) — used when recState==Playback
};

// ─── LfoEngine class ──────────────────────────────────────────────────────────
class LfoEngine
{
public:
    LfoEngine()  = default;
    ~LfoEngine() = default;

    // Audio-thread call — returns LFO output float in [-level, +level].
    // Call once per processBlock(). Not thread-safe with reset().
    float process(const ProcessParams& p);

    // Reset all state to initial values.
    // Call from prepareToPlay() and on transport stop.
    // NOT audio-thread safe with simultaneous process() call.
    void reset();

    // ── Recording state machine ───────────────────────────────────────────────
    // Transitions driven by processBlock (audio thread) and PluginEditor via
    // PluginProcessor (message thread).
    void arm();            // Unarmed→Armed or Playback→Armed (re-arm). Message thread.
    void clearRecording(); // Any state→Unarmed, clears buffer. Message thread.
    void startCapture();   // Armed→Recording, resets write head. Audio thread only.
    void stopCapture();    // Recording→Playback. Audio thread only.

    // State read — message thread polls this in timerCallback().
    LfoRecState getRecState() const
    {
        return static_cast<LfoRecState>(recState_.load(std::memory_order_relaxed));
    }

    // Request a phase reset on the next process() call (message-thread safe).
    void requestPhaseReset() { phaseResetPending_.store(true, std::memory_order_relaxed); }

    // Consume a pending phase reset immediately, even if LFO is disabled.
    // Call unconditionally every processBlock so the reset takes effect on OFF, not ON.
    void applyPhaseResetIfPending()
    {
        if (phaseResetPending_.exchange(false, std::memory_order_relaxed))
        {
            phase_       = 0.0;
            sampleCount_ = 0;
            totalCycles_ = 0.0;
        }
    }

private:
    // ── Timing state ─────────────────────────────────────────────────────────
    double   phase_          = 0.0;    // normalized phase accumulator, free mode [0.0, 1.0)
    int64_t  sampleCount_    = 0;      // sample counter for sync mode with DAW stopped
    double   totalCycles_    = 0.0;    // accumulated total cycle count; free-mode S&H boundary

    // ── S&H state ─────────────────────────────────────────────────────────────
    float    shHeld_         = 0.0f;   // S&H held value (constant within a cycle)
    int64_t  prevCycle_      = -1;     // previous integer cycle index; -1 sentinel so first
                                       // block ALWAYS fires S&H (guarantees a non-zero shHeld_)

    // ── Transport state ───────────────────────────────────────────────────────
    bool     prevDawPlaying_ = false;  // for transport restart detection

    // ── LCG random number generator ───────────────────────────────────────────
    // One shared LCG per instance — used for S&H, Random waveform, and distortion noise.
    // Non-zero seed; any value is fine as long as it is not 0.
    uint32_t rng_            = 0xABCDEF01u;

    // Returns a value in [-1, +1] using signed-integer division.
    //
    // IMPORTANT: This differs from TriggerSystem::nextRandom() which maps rng_ >> 1
    // to [0, 1). LfoEngine maps as signed int32 / 0x7FFFFFFF to get a bipolar signal
    // suitable for direct use as an LFO waveform output.
    inline float nextLcg()
    {
        rng_ = rng_ * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(rng_)) / float(0x7FFFFFFF);
    }

    // Evaluates the waveform at normalized phase phi in [0, 1).
    // Non-const because Random waveform calls nextLcg() which mutates rng_.
    float evaluateWaveform(Waveform w, float phi);

    // ── LFO recording ring buffer ─────────────────────────────────────────────
    static constexpr int kRecBufSize = 4096;
    float recBuf_[kRecBufSize] = {};
    int   captureWriteIdx_     = 0;     // audio-thread-only write position
    int   capturedCount_       = 0;     // audio-thread-only written sample count
    float lastRecValue_        = 0.0f;  // previous block's captured value for sub-block interpolation

    // Atomic int (not atomic<LfoRecState>) — avoids MSVC C2338 on enum class in atomics.
    // Cast to/from LfoRecState on every store/load.
    std::atomic<int>  recState_          { 0 };     // 0 = Unarmed
    std::atomic<bool> phaseResetPending_ { false };  // set by message thread, consumed in process()
};
