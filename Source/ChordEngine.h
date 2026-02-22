#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ScaleQuantizer.h"

// Computes 4-voice MIDI pitches from joystick position + parameters.
//
// Voice layout:
//   0 = Root     (Y axis)
//   1 = Third    (Y axis + thirdInterval)
//   2 = Fifth    (X axis + fifthInterval)
//   3 = Tension  (X axis + tensionInterval)
//
// Sample-and-hold: the caller asks for computePitch() at trigger time,
// stores the result, and holds it until the next trigger.

class ChordEngine
{
public:
    struct Params
    {
        // Joystick axes, normalised -1..+1
        // Default: X = center (0.0), Y = bottom (-1.0 = lowest pitch)
        float joystickX = 0.0f;
        float joystickY = -1.0f;

        // Attenuators map full-range joystick → 0..atten MIDI semitones
        float xAtten = 24.0f;  // default 2 octave range (X axis)
        float yAtten = 12.0f;  // default 1 octave range (Y axis)

        // Global transpose (−24..+24 semitones)
        int globalTranspose = 0;

        // Interval offsets above root, semitones [voice 0..3]
        // voice 0 = root → interval[0] is always 0
        int intervals[4] = {0, 4, 7, 10};

        // Octave shifts per voice (0..12), default 2
        int octaves[4] = {2, 2, 2, 2};

        // Scale quantization
        bool        useCustomScale = false;
        bool        customNotes[12] = {};
        ScalePreset scalePreset    = ScalePreset::Major;
    };

    // Compute MIDI pitch (0–127) for given voice at this moment.
    static int computePitch(int voiceIndex, const Params& p);

private:
    // Raw (unquantized) pitch for voice index
    static float rawPitch(int voiceIndex, const Params& p);
};
