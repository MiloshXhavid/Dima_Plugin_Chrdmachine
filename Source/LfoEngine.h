#pragma once
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
};
