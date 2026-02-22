#include "TriggerSystem.h"
#include <cmath>

TriggerSystem::TriggerSystem()
{
    for (auto& a : padPressed_)   a.store(false);
    for (auto& a : padJustFired_) a.store(false);
    for (auto& a : gateOpen_)     a.store(false);
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
    const double samplesPerBeat  = (p.sampleRate * 60.0) / p.bpm;
    const double subdivBeats     = subdivisionBeats(p.randomSubdiv);
    const double samplesPerSubdiv = samplesPerBeat * subdivBeats;

    // ── Joystick continuous gate model ───────────────────────────────────────
    // Magnitude is Chebyshev distance of per-block delta (not absolute position).
    const float dx = p.joystickX - static_cast<float>(prevJoystickX_);
    const float dy = p.joystickY - static_cast<float>(prevJoystickY_);
    prevJoystickX_ = p.joystickX;
    prevJoystickY_ = p.joystickY;
    const float magnitude         = std::max(std::abs(dx), std::abs(dy));  // Chebyshev distance
    const bool  joyAboveThreshold = magnitude > p.joystickThreshold;
    // 500ms stillness window for retrigger check (NOT a gate-close timer)
    const int   retriggerSamples  = static_cast<int>(0.500 * p.sampleRate);

    // ── Per-sample random clock ───────────────────────────────────────────────
    // We advance the random clock and check for subdivision crossings.
    // For simplicity we fire once at the start of the block if a subdivision
    // boundary fell within this block.  Full sample-accurate scheduling is a
    // future enhancement.

    const bool allTrig = allTrigger_.exchange(false);

    // Check if a subdivision boundary crossed this block
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
        const int       ch    = p.midiChannels[v] - 1;   // 0-based for MidiBuffer
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
                fireNoteOff(v, ch, 0, p);
            }
        }
        else if (src == TriggerSource::Joystick)
        {
            if (joyAboveThreshold)
            {
                // Joystick is moving — reset stillness counter
                joystickStillSamples_[v] = 0;

                if (!gateOpen_[v].load())
                {
                    // Gate was closed — open it and record the sounding pitch
                    int pitch = p.heldPitches[v];
                    fireNoteOn(v, pitch, ch, 0, p);
                    joyActivePitch_[v] = pitch;
                    // trigger flag already handled by fireNoteOn — skip common trigger path
                }
                // Gate already open: hold note, no retrigger during motion
            }
            else
            {
                // Joystick is still — accumulate stillness samples
                joystickStillSamples_[v] += p.blockSize;

                if (gateOpen_[v].load() && joystickStillSamples_[v] >= retriggerSamples)
                {
                    // 500ms of stillness — check if quantized pitch has changed
                    int currentPitch = p.heldPitches[v];
                    if (currentPitch != joyActivePitch_[v])
                    {
                        // Pitch changed — retrigger: note-off then note-on at new pitch
                        fireNoteOff(v, ch, 0, p);
                        fireNoteOn(v, currentPitch, ch, 0, p);
                        joyActivePitch_[v] = currentPitch;
                    }
                    // Reset counter so we don't retrigger again next block
                    joystickStillSamples_[v] = 0;
                }
                // Gate stays open indefinitely — never closed by stillness alone
            }
            // Skip the common trigger path for Joystick source (handled inline above)
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
                fireNoteOff(v, ch, 0, p);   // stop previous note first
            fireNoteOn(v, p.heldPitches[v], ch, 0, p);
        }
    }

    // joystickTrig_ is kept for external callers (notifyJoystickMoved) but the
    // continuous gate model in the per-voice loop no longer uses it here.
    joystickTrig_.store(false);
}

void TriggerSystem::resetAllGates()
{
    for (int v = 0; v < 4; ++v)
    {
        gateOpen_[v].store(false);
        activePitch_[v] = -1;
        padPressed_[v].store(false);
        padJustFired_[v].store(false);
        joystickStillSamples_[v] = 0;
        joyActivePitch_[v]       = -1;
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
