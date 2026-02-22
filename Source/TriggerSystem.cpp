#include "TriggerSystem.h"
#include <cmath>

TriggerSystem::TriggerSystem()
{
    for (auto& a : padPressed_)   a.store(false);
    for (auto& a : padJustFired_) a.store(false);
    for (auto& a : gateOpen_)     a.store(false);
    joyActivePitch_.fill(-1);
    joystickStillSamples_.fill(0);
    joyPendingPitch_.fill(-1);
    joyPendingSamples_.fill(0);
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

static double subdivisionBeats(RandomSubdiv s)
{
    switch (s)
    {
        case RandomSubdiv::Quarter:      return 1.0;
        case RandomSubdiv::Eighth:       return 0.5;
        case RandomSubdiv::Sixteenth:    return 0.25;
        case RandomSubdiv::ThirtySecond: return 0.125;
    }
    return 0.5;
}

void TriggerSystem::processBlock(const ProcessParams& p)
{
    const double samplesPerBeat   = (p.sampleRate * 60.0) / p.bpm;
    const double subdivBeats      = subdivisionBeats(p.randomSubdiv);
    const double samplesPerSubdiv = samplesPerBeat * subdivBeats;

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

    // ── Per-sample random clock ───────────────────────────────────────────────
    const bool allTrig = allTrigger_.exchange(false);

    bool randomFired = false;
    randomPhase_ += static_cast<double>(p.blockSize);
    if (randomPhase_ >= samplesPerSubdiv && samplesPerSubdiv > 0.0)
    {
        randomPhase_ = std::fmod(randomPhase_, samplesPerSubdiv);
        randomFired = true;
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

            if (joyAboveThreshold)
            {
                // Axis is active — reset gate-close stillness counter.
                joystickStillSamples_[v] = 0;

                if (!gateOpen_[v].load())
                {
                    // ── Gate OPENS ────────────────────────────────────────────
                    const int pitch = p.heldPitches[v];
                    fireNoteOn(v, pitch, ch - 1, 0, p);   // sets gateOpen_ and activePitch_
                    joyActivePitch_[v]    = pitch;
                    joyPendingPitch_[v]   = -1;
                    joyPendingSamples_[v] = 0;
                }
                else
                {
                    // ── Gate OPEN + above threshold ───────────────────────────
                    // Retrigger only after the new pitch has been stable for 30ms.
                    const int newPitch    = p.heldPitches[v];
                    const int debounce    = static_cast<int>(0.030f * static_cast<float>(p.sampleRate));

                    if (newPitch == joyActivePitch_[v])
                    {
                        // Still on same pitch — reset pending candidate.
                        joyPendingPitch_[v]   = -1;
                        joyPendingSamples_[v] = 0;
                    }
                    else
                    {
                        if (newPitch == joyPendingPitch_[v])
                        {
                            // Same candidate as last block — accumulate dwell time.
                            joyPendingSamples_[v] += p.blockSize;

                            if (joyPendingSamples_[v] >= debounce)
                            {
                                // Pitch committed after 30ms — retrigger.
                                fireNoteOff(v, ch - 1, 0, p);
                                fireNoteOn (v, newPitch, ch - 1, 0, p);
                                joyActivePitch_[v]    = newPitch;
                                joyPendingPitch_[v]   = -1;
                                joyPendingSamples_[v] = 0;
                            }
                        }
                        else
                        {
                            // New candidate pitch — start fresh debounce.
                            joyPendingPitch_[v]   = newPitch;
                            joyPendingSamples_[v] = 0;
                        }
                    }
                }
            }
            else
            {
                // Below open threshold — apply hysteresis:
                // Only count toward gate-close when the axis is near rest
                // (< 15% of open threshold).  A brief dip just below threshold
                // (e.g. mid-movement) resets the counter instead, keeping the
                // gate open and preventing spurious same-pitch retriggering.
                const float closeThreshold = p.joystickThreshold * 0.15f;
                const int   closeAfter     = static_cast<int>(0.050f * static_cast<float>(p.sampleRate)); // 50ms debounce

                if (axisMag < closeThreshold)
                {
                    // Axis is near rest — accumulate stillness counter.
                    joystickStillSamples_[v] += p.blockSize;

                    if (gateOpen_[v].load() && joystickStillSamples_[v] >= closeAfter)
                    {
                        // Axis has been near rest for 50ms — close the gate.
                        fireNoteOff(v, ch - 1, 0, p);   // clears gateOpen_ and activePitch_
                        joyActivePitch_[v]       = -1;
                        joystickStillSamples_[v] = 0;
                        joyPendingPitch_[v]      = -1;
                        joyPendingSamples_[v]    = 0;
                    }
                }
                else
                {
                    // Magnitude between closeThreshold and joystickThreshold —
                    // axis is hovering near threshold mid-movement.
                    // Reset the stillness counter so a brief dip doesn't
                    // eventually close the gate.
                    joystickStillSamples_[v] = 0;
                }
            }

            // Skip the common trigger path for Joystick source (handled inline above).
            trigger = false;
        }
        else if (src == TriggerSource::Random)
        {
            if (randomFired && nextRandom() < p.randomDensity)
                trigger = true;
        }

        if (trigger)
        {
            if (gateOpen_[v].load())
                fireNoteOff(v, ch - 1, 0, p);   // stop previous note first
            fireNoteOn(v, p.heldPitches[v], ch - 1, 0, p);
        }
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
        joystickStillSamples_[v] = 0;
        joyPendingPitch_[v]      = -1;
        joyPendingSamples_[v]    = 0;
        padPressed_[v].store(false);
        padJustFired_[v].store(false);
    }
    allTrigger_.store(false);
    joystickTrig_.store(false);
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
