#include "LfoEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

// Use a local constexpr to avoid any JUCE header dependency in this translation unit.
static constexpr float kTwoPi = 6.28318530718f;

// ─── reset() ──────────────────────────────────────────────────────────────────
// Restores all mutable state to initial values.
// Call from prepareToPlay() and whenever the transport is stopped.
void LfoEngine::reset()
{
    phase_          = 0.0;
    sampleCount_    = 0;
    totalCycles_    = 0.0;
    shHeld_         = 0.0f;
    prevCycle_      = -1;      // -1 sentinel ensures S&H fires on the very first block
    prevDawPlaying_ = false;
    // Recording state is intentionally NOT reset here — reset() is called on
    // transport stop, but per CONTEXT.md "Looper Stop → LFO keeps its current state".
    // clearRecording() is called explicitly when looper clear fires.
}

// ─── process() ────────────────────────────────────────────────────────────────
// Audio-thread entry point. Returns LFO output float in [-level, +level].
// Called once per processBlock(). Never writes to shared atomics.
float LfoEngine::process(const ProcessParams& p)
{
    // ── 1. Clamp subdivision to configured maximum ────────────────────────────
    // In free mode cycleBeats is unused (set to 1.0 as a placeholder).
    const double cycleBeats = p.syncMode
        ? std::min(p.subdivBeats, p.maxCycleBeats)
        : 1.0;

    // ── 2. Pre-compute timing constants ───────────────────────────────────────
    // cycleSeconds / cycleSamples are used by the sync+stopped branch (Branch B).
    // For free mode, freeModeCycleSamples is used instead.
    const double cycleSeconds       = cycleBeats * (60.0 / p.bpm);
    const double cycleSamples       = cycleSeconds * p.sampleRate;
    const double freeModeCycleSamples = (1.0 / static_cast<double>(p.rateHz)) * p.sampleRate;

    // ── 3. Transport restart detection ────────────────────────────────────────
    // Must run BEFORE the phase derivation branches so sampleCount_ is reset
    // before Branch B might use it.
    if (!prevDawPlaying_ && p.isDawPlaying)
        sampleCount_ = 0;   // reset free/stopped counter on DAW restart
    prevDawPlaying_ = p.isDawPlaying;

    // ── 3b. Phase reset request (from LFO ON button click) ────────────────────
    if (phaseResetPending_.exchange(false, std::memory_order_relaxed))
    {
        phase_       = 0.0;
        sampleCount_ = 0;
        totalCycles_ = 0.0;
    }

    // ── 4. Phase derivation — three branches ──────────────────────────────────
    double normalizedPhase = 0.0;
    int64_t currentCycle   = 0;

    if (p.syncMode && p.isDawPlaying && p.ppqPosition >= 0.0)
    {
        // Branch A: Sync + DAW playing
        // Phase is derived directly from ppqPosition to prevent drift.
        normalizedPhase = std::fmod(p.ppqPosition, cycleBeats) / cycleBeats;

        // Keep sampleCount_ in sync for a seamless DAW-stop fallback (Branch B).
        sampleCount_ = static_cast<int64_t>(normalizedPhase * cycleSamples);

        // S&H cycle index from ppqPos — integer index of current cycle.
        currentCycle = static_cast<int64_t>(p.ppqPosition / cycleBeats);
    }
    else if (p.syncMode && !p.isDawPlaying)
    {
        // Branch B: Sync + DAW stopped
        // Continue LFO using sample counter so motion does not freeze when
        // the DAW is paused.
        normalizedPhase = std::fmod(static_cast<double>(sampleCount_), cycleSamples) / cycleSamples;
        sampleCount_ += p.blockSize;
        currentCycle = static_cast<int64_t>(sampleCount_ / cycleSamples);
    }
    else
    {
        // Branch C: Free mode
        // Accumulate phase_ per block using rateHz / sampleRate — no ppqPos dependency.
        const double phaseIncrement = (static_cast<double>(p.rateHz) / p.sampleRate) * p.blockSize;
        phase_       = std::fmod(phase_ + phaseIncrement, 1.0);
        normalizedPhase = phase_;
        totalCycles_ += phaseIncrement;
        currentCycle  = static_cast<int64_t>(totalCycles_);

        // (freeModeCycleSamples is computed above but not used in this branch;
        //  it exists as documentation and for potential future use.)
        (void)freeModeCycleSamples;
    }

    // ── 5. S&H boundary detection ─────────────────────────────────────────────
    // A new random value is latched whenever the integer cycle index changes.
    // No interpolation — shHeld_ is constant for the entire cycle.
    // prevCycle_ starts at -1 so the very first block always triggers a latch.
    if (p.waveform == Waveform::SH)
    {
        if (currentCycle != prevCycle_)
        {
            shHeld_    = nextLcg();   // instant jump, no interpolation
            prevCycle_ = currentCycle;
        }
    }

    // ── 6. Apply phase shift ───────────────────────────────────────────────────
    // Phase shift is applied after computing normalizedPhase so it works
    // identically across all three timing branches.
    normalizedPhase = std::fmod(normalizedPhase + static_cast<double>(p.phaseShift), 1.0);
    if (normalizedPhase < 0.0)
        normalizedPhase += 1.0;  // guard against negative fmod result on some platforms

    // ── 7. Evaluate waveform ──────────────────────────────────────────────────
    float output = evaluateWaveform(p.waveform, static_cast<float>(normalizedPhase));

    // ── 9. Apply level / depth (moved before distortion so capture stores post-level value)
    // recBuf_ stores post-level, pre-distortion values. This satisfies LFOREC-06:
    // Distort is applied live and never recorded.
    output *= p.level;

    // ── Recording capture / Playback read ────────────────────────────────────
    {
        const int state = recState_.load(std::memory_order_relaxed);
        if (state == static_cast<int>(LfoRecState::Recording))
        {
            // Write 8 linearly-interpolated values between the previous block's output
            // and the current block's output. At 512-sample blocks / 44100 Hz this gives
            // ~8x denser capture (~2752 values for a 2-bar loop at 120 BPM) compared to
            // writing a single value per block (~344 values), eliminating visible choppiness
            // during playback interpolation.
            // NOTE: state transition Recording->Playback is driven by processBlock()
            // edge-detecting looper_.isRecording() false edge. Do NOT self-transition here.
            static constexpr int kSubBlockWrites = 8;
            for (int i = 0; i < kSubBlockWrites; ++i)
            {
                if (captureWriteIdx_ >= kRecBufSize)
                    break;
                const float t = (i + 1) * (1.0f / static_cast<float>(kSubBlockWrites));
                recBuf_[captureWriteIdx_++] = lastRecValue_ + t * (output - lastRecValue_);
            }
            lastRecValue_ = output;
            capturedCount_ = captureWriteIdx_;
        }
        else if (state == static_cast<int>(LfoRecState::Playback))
        {
            // playbackPhase is [0.0, 1.0) — set by processBlock from looper beat position.
            // capturedCount_ is the number of valid samples written during recording.
            // If nothing was captured (capturedCount_ == 0), output stays as live waveform
            // (safe fallback — should not happen if processor logic is correct).
            if (capturedCount_ > 0)
            {
                const int   validCount = std::min(capturedCount_, kRecBufSize);
                const float fIdx       = p.playbackPhase * static_cast<float>(validCount);
                const int   i0         = static_cast<int>(fIdx) % validCount;
                const int   i1         = (i0 + 1) % validCount;
                const float frac       = fIdx - static_cast<float>(static_cast<int>(fIdx));
                output = recBuf_[i0] + frac * (recBuf_[i1] - recBuf_[i0]);
                // Do NOT multiply output by p.level — recBuf_ already contains post-level values.
            }
        }
        // Unarmed or Armed: output is the live waveform (no modification).
    }

    // ── 8. Distortion — additive LCG noise (always applied, including during Playback) ──
    // distortion = 0 → bypass. This is the LIVE distortion application (LFOREC-06).
    // Applied to all waveforms EXCEPT Random (Random is already pure noise;
    // layering more noise on it is a no-op per CONTEXT.md).
    if (p.waveform != Waveform::Random && p.distortion > 0.0f)
        output += p.distortion * nextLcg();

    return output;
}

// ─── evaluateWaveform() ───────────────────────────────────────────────────────
// Maps normalized phase phi in [0, 1) to a waveform value.
// Non-const because the Random case calls nextLcg() which mutates rng_.
float LfoEngine::evaluateWaveform(Waveform w, float phi)
{
    switch (w)
    {
        case Waveform::Sine:
            // Standard sine: 0→0, 0.25→+1, 0.5→0, 0.75→-1, 1→0
            return std::sin(phi * kTwoPi);                        // [-1, +1]

        case Waveform::Triangle:
            // phi=0→-1, phi=0.25→+1, phi=0.5→-1, phi=0.75→+1
            // Rising half: 4*phi - 1   (0→-1, 0.5→1)
            // Falling half: 3 - 4*phi  (0.5→1, 1→-1)
            return (phi < 0.5f) ? (4.0f * phi - 1.0f) : (3.0f - 4.0f * phi);

        case Waveform::SawUp:
            // Ramps from -1 to +1 over one cycle.
            return 2.0f * phi - 1.0f;                             // -1→+1

        case Waveform::SawDown:
            // Ramps from +1 to -1 over one cycle (inverse sawtooth).
            return 1.0f - 2.0f * phi;                             // +1→-1

        case Waveform::Square:
            // Simple two-state wave; 50% duty cycle.
            return (phi < 0.5f) ? 1.0f : -1.0f;

        case Waveform::SH:
            // shHeld_ is set by the S&H boundary detection block in process().
            // It remains constant for the full cycle — no interpolation.
            return shHeld_;

        case Waveform::Random:
            // Per-sample LCG white noise — rate-agnostic.
            // distortion is NOT applied on top of Random (see process() step 8).
            return nextLcg();

        default:
            return 0.0f;
    }
}

// ─── Recording state machine ───────────────────────────────────────────────────

void LfoEngine::arm()
{
    // If already Armed, cancel arming (Armed->Unarmed toggle).
    // This lets players undo an accidental ARM press before recording starts.
    // Use CAS to avoid clobbering a concurrent Recording->Playback transition.
    int expected = static_cast<int>(LfoRecState::Armed);
    if (recState_.compare_exchange_strong(expected, static_cast<int>(LfoRecState::Unarmed),
                                          std::memory_order_relaxed, std::memory_order_relaxed))
        return;

    // Unarmed->Armed or Playback->Armed (re-arm overwrites on next record cycle).
    // Called from message thread via PluginProcessor passthrough.
    recState_.store(static_cast<int>(LfoRecState::Armed), std::memory_order_relaxed);
}

void LfoEngine::clearRecording()
{
    // Any state->Unarmed. Does NOT zero recBuf_ (preserving old data is harmless).
    recState_.store(static_cast<int>(LfoRecState::Unarmed), std::memory_order_relaxed);
    captureWriteIdx_ = 0;
    capturedCount_   = 0;
    lastRecValue_    = 0.0f;
}

void LfoEngine::startCapture()
{
    // Armed->Recording. Resets write head. Called from audio thread only.
    captureWriteIdx_ = 0;
    capturedCount_   = 0;
    lastRecValue_    = 0.0f;
    recState_.store(static_cast<int>(LfoRecState::Recording), std::memory_order_relaxed);
}

void LfoEngine::stopCapture()
{
    // Recording→Playback. Called from audio thread only (processBlock falling edge).
    recState_.store(static_cast<int>(LfoRecState::Playback), std::memory_order_relaxed);
}
