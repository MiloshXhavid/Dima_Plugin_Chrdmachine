#include "TriggerSystem.h"
#include <cmath>

TriggerSystem::TriggerSystem()
{
    for (auto& a : padPressed_)   a.store(false);
    for (auto& a : padJustFired_) a.store(false);
    for (auto& a : gateOpen_)     a.store(false);
    joyActivePitch_.fill(-1);
    joystickStillSamples_.fill(0);
    joyOpenPitch_.fill(-1);
    joyLastBend_.fill(0);
}

// ── UI / gamepad thread ───────────────────────────────────────────────────────

void TriggerSystem::setPadState(int voice, bool pressed)
{
    if (voice < 0 || voice >= 4) return;

    const bool wasPressed = padPressed_[voice].exchange(pressed);
    if (pressed && !wasPressed)
        padJustFired_[voice].store(true);  // rising edge
}

void TriggerSystem::notifyJoystickMoved()
{
    joystickTrig_.store(true);
}

void TriggerSystem::triggerAllNotes()
{
    allTrigger_.store(true);
}

// ── Audio thread ──────────────────────────────────────────────────────────────

// Subdivision helper lambdas (file-scope statics, computed once)
static auto subdivBeatsFor = [](RandomSubdiv s) -> double {
    switch (s) {
        case RandomSubdiv::Quarter:      return 1.0;
        case RandomSubdiv::Eighth:       return 0.5;
        case RandomSubdiv::Sixteenth:    return 0.25;
        case RandomSubdiv::ThirtySecond: return 0.125;
    }
    return 0.5;
};

static float hitsPerBarToProbability(float density, RandomSubdiv subdiv)
{
    const float subdivsPerBar = static_cast<float>(
        subdiv == RandomSubdiv::Quarter ? 4 :
        subdiv == RandomSubdiv::Eighth  ? 8 :
        subdiv == RandomSubdiv::Sixteenth ? 16 : 32);
    return juce::jlimit(0.0f, 1.0f, density / subdivsPerBar);
}

void TriggerSystem::processBlock(const ProcessParams& p)
{
    // ── Per-axis delta magnitude for per-voice gate detection ─────────────────
    // Gate detection uses how much the joystick MOVED this block (delta),
    // not its absolute position.  A joystick resting at Y=-1.0 has absY=1.0
    // which would permanently exceed any threshold; delta is 0 when still.
    //
    // Voices 0+1 (Root/Third) → Y-axis delta
    // Voices 2+3 (Fifth/Tension) → X-axis delta
    //
    // prevJoystickX_/Y_ are kept in sync here so legacy callers still compile,
    // but the JOY gate now uses the pre-computed deltaX/deltaY from ProcessParams.
    prevJoystickX_ = p.joystickX;
    prevJoystickY_ = p.joystickY;

    const float absDeltaX = std::abs(p.deltaX);
    const float absDeltaY = std::abs(p.deltaY);

    // Returns the relevant axis delta magnitude for a given voice
    auto axisForVoice = [&](int v) -> float {
        return (v < 2) ? absDeltaY : absDeltaX;
    };

    // ── Transport restart detection ───────────────────────────────────────────
    // Reset subdivision indices on transport restart to avoid stale index
    if (p.isDawPlaying && !wasPlaying_)
    {
        for (int v = 0; v < 4; ++v)
            prevSubdivIndex_[v] = -1;
    }
    wasPlaying_ = p.isDawPlaying;

    // ── Per-voice random subdivision clock ────────────────────────────────────
    const bool allTrig = allTrigger_.exchange(false);

    bool randomFired[4] = {};
    for (int v = 0; v < 4; ++v)
    {
        const double effectiveBpm     = p.randomClockSync
                                            ? p.bpm
                                            : static_cast<double>(p.randomFreeTempo);
        const double beats            = subdivBeatsFor(p.randomSubdiv[v]);
        const double beatsPerSec      = effectiveBpm / 60.0;
        const double samplesPerSubdiv = (p.sampleRate / beatsPerSec) * beats;

        if (p.randomClockSync)
        {
            // Sync mode: fire on ppq subdivision boundary, only when DAW is playing.
            // When DAW stops, do nothing — no fallback clock.
            if (p.isDawPlaying && p.ppqPosition >= 0.0)
            {
                const int64_t idx = static_cast<int64_t>(p.ppqPosition / beats);
                if (idx != prevSubdivIndex_[v])
                {
                    prevSubdivIndex_[v] = idx;
                    randomFired[v] = true;
                }
            }
            // else: transport stopped — no trigger, no phase advance
        }
        else
        {
            // Free mode: always-running sample-count clock at randomFreeTempo.
            randomPhase_[v] += static_cast<double>(p.blockSize);
            if (samplesPerSubdiv > 0.0 && randomPhase_[v] >= samplesPerSubdiv)
            {
                randomPhase_[v] = std::fmod(randomPhase_[v], samplesPerSubdiv);
                randomFired[v] = true;
            }
        }
    }

    // ── Per-voice processing ─────────────────────────────────────────────────
    for (int v = 0; v < 4; ++v)
    {
        const int         ch  = p.midiChannels[v];   // 1-based (JUCE MidiMessage convention)
        const TriggerSource src = p.sources[v];
        bool trigger = false;

        if (allTrig)
        {
            trigger = true;
        }
        else if (src == TriggerSource::TouchPlate)
        {
            const bool pressed   = padPressed_[v].load();
            const bool justFired = padJustFired_[v].exchange(false);

            if (justFired)
            {
                trigger = true;
            }
            else if (!pressed && gateOpen_[v].load())
            {
                // Released → note off
                fireNoteOff(v, ch - 1, 0, p);   // fireNoteOff expects 0-based ch
            }
        }
        else if (src == TriggerSource::Joystick)
        {
            const float axisMag           = axisForVoice(v);
            const bool  joyAboveThreshold = axisMag > p.joystickThreshold;

            // Pitch bend range for JOY mode: ±kBendSemitones semitones maps to full 14-bit range.
            static constexpr int kBendSemitones = 24;

            if (joyAboveThreshold)
            {
                // Axis is moving — reset gate-close stillness counter.
                joystickStillSamples_[v] = 0;

                if (!gateOpen_[v].load())
                {
                    // ── Gate OPENS ────────────────────────────────────────────
                    const int pitch = p.heldPitches[v];
                    // Reset pitch bend to centre before note-on so the synth starts clean.
                    if (p.onPitchBend) p.onPitchBend(v, 0, ch - 1, 0);
                    joyLastBend_[v] = 0;
                    fireNoteOn(v, pitch, ch - 1, 0, p);
                    joyActivePitch_[v] = pitch;
                    joyOpenPitch_[v]   = pitch;
                }
                else
                {
                    // ── Gate OPEN + moving: pitch CV via pitch bend ───────────
                    // No retrigger — shift pitch by bending from the open-pitch reference.
                    if (p.onPitchBend && joyOpenPitch_[v] >= 0)
                    {
                        const int semiOffset = p.heldPitches[v] - joyOpenPitch_[v];
                        const int bendVal    = juce::jlimit(-8192, 8191,
                            static_cast<int>(semiOffset * (8191.0f / (float)kBendSemitones)));
                        if (bendVal != joyLastBend_[v])
                        {
                            p.onPitchBend(v, bendVal, ch - 1, 0);
                            joyLastBend_[v] = bendVal;
                        }
                    }
                }
            }
            else
            {
                // Below open threshold — hysteresis: only count toward gate-close
                // when axis is very near rest (< 15% of threshold).
                const float closeThreshold = p.joystickThreshold * 0.15f;
                const int   closeAfter     = static_cast<int>(1.0f * static_cast<float>(p.sampleRate)); // 1 second

                if (axisMag < closeThreshold)
                {
                    joystickStillSamples_[v] += p.blockSize;

                    if (gateOpen_[v].load() && joystickStillSamples_[v] >= closeAfter)
                    {
                        // 1 second of stillness — reset bend then close gate.
                        if (p.onPitchBend && joyLastBend_[v] != 0)
                        {
                            p.onPitchBend(v, 0, ch - 1, 0);
                            joyLastBend_[v] = 0;
                        }
                        fireNoteOff(v, ch - 1, 0, p);
                        joyActivePitch_[v]       = -1;
                        joyOpenPitch_[v]         = -1;
                        joystickStillSamples_[v] = 0;
                    }
                }
                else
                {
                    // Hovering near threshold — reset counter to prevent premature close.
                    joystickStillSamples_[v] = 0;
                }
            }

            // Skip the common trigger path — JOY source handled inline above.
            trigger = false;
        }
        else if (src == TriggerSource::Random)
        {
            if (randomFired[v])
            {
                const float prob = hitsPerBarToProbability(p.randomDensity, p.randomSubdiv[v]);
                if (nextRandom() < prob)
                    trigger = true;
            }
        }

        if (trigger)
        {
            if (src == TriggerSource::Random)
            {
                // Random source: fire note-on and start gate-time countdown
                if (gateOpen_[v].load())
                    fireNoteOff(v, ch - 1, 0, p);
                fireNoteOn(v, p.heldPitches[v], ch - 1, 0, p);

                const double beats            = subdivBeatsFor(p.randomSubdiv[v]);
                const double beatsPerSec      = p.bpm / 60.0;
                const double samplesPerSubdiv = (p.sampleRate / beatsPerSec) * beats;
                const int minDurationSamples  = static_cast<int>(0.010 * p.sampleRate); // 10ms floor
                randomGateRemaining_[v] = std::max(minDurationSamples,
                    static_cast<int>(p.randomGateTime * samplesPerSubdiv));
            }
            else
            {
                if (gateOpen_[v].load())
                    fireNoteOff(v, ch - 1, 0, p);   // stop previous note first
                fireNoteOn(v, p.heldPitches[v], ch - 1, 0, p);
            }
        }

        // Auto note-off countdown for RND source
        if (src == TriggerSource::Random && gateOpen_[v].load() && randomGateRemaining_[v] > 0)
        {
            randomGateRemaining_[v] -= p.blockSize;
            if (randomGateRemaining_[v] <= 0)
            {
                randomGateRemaining_[v] = 0;
                fireNoteOff(v, ch - 1, p.blockSize - 1, p);
            }
        }
        // Clear countdown if mode is not RND (prevents ghost note-off on mode switch)
        if (src != TriggerSource::Random)
            randomGateRemaining_[v] = 0;
    }

    // joystickTrig_ is kept for external callers (notifyJoystickMoved) but the
    // continuous gate model in the per-voice loop no longer uses it.
    joystickTrig_.store(false);
}

void TriggerSystem::resetAllGates()
{
    for (int v = 0; v < 4; ++v)
    {
        // Any open JOY gate is silenced here.
        // Note: the caller (processBlockBypassed) fires noteOff via getActivePitch()
        // before calling this, so no MIDI is sent here.
        gateOpen_[v].store(false);
        activePitch_[v]          = -1;
        joyActivePitch_[v]       = -1;
        joyOpenPitch_[v]         = -1;
        joyLastBend_[v]          = 0;
        joystickStillSamples_[v] = 0;
        padPressed_[v].store(false);
        padJustFired_[v].store(false);

        // Per-voice random clock state
        randomPhase_[v]         = 0.0;
        prevSubdivIndex_[v]     = -1;
        randomGateRemaining_[v] = 0;
    }
    allTrigger_.store(false);
    joystickTrig_.store(false);
    wasPlaying_ = false;
}

void TriggerSystem::fireNoteOn(int voice, int pitch, int ch, int sampleOff,
                                const ProcessParams& p)
{
    if (pitch < 0 || pitch > 127) return;
    activePitch_[voice] = pitch;
    gateOpen_[voice].store(true);
    p.onNote(voice, pitch, true, sampleOff);
}

void TriggerSystem::fireNoteOff(int voice, int ch, int sampleOff,
                                 const ProcessParams& p)
{
    const int pitch = activePitch_[voice];
    if (pitch < 0) return;
    gateOpen_[voice].store(false);
    activePitch_[voice] = -1;
    p.onNote(voice, pitch, false, sampleOff);
}
