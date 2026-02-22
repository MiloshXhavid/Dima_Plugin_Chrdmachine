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

    // ── Detect joystick movement ─────────────────────────────────────────────
    const float  dx         = p.joystickX - static_cast<float>(prevJoystickX_);
    const float  dy         = p.joystickY - static_cast<float>(prevJoystickY_);
    const bool   joyMoved   = (std::abs(dx) + std::abs(dy)) > 0.005f;
    prevJoystickX_ = p.joystickX;
    prevJoystickY_ = p.joystickY;

    if (joyMoved)
        joystickTrig_.store(true);

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
            if (joystickTrig_.load())
                trigger = true;
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

    // Joystick trig is consumed once all joystick-source voices have been processed
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
