#include <catch2/catch_test_macros.hpp>
#include "ChordEngine.h"

// -- Test helper ---------------------------------------------------------------
// Params with joystick centered, Chromatic scale (no quantization rounding),
// default intervals {0,4,7,10}, no transpose, no octave shifts.
static ChordEngine::Params baseParams()
{
    ChordEngine::Params p;
    p.joystickX       = 0.0f;
    p.joystickY       = 0.0f;
    p.xAtten          = 48.0f;
    p.yAtten          = 48.0f;
    p.globalTranspose = 0;
    p.intervals[0]    = 0;
    p.intervals[1]    = 4;
    p.intervals[2]    = 7;
    p.intervals[3]    = 10;
    for (auto& o : p.octaves) o = 0;
    p.useCustomScale  = false;
    p.scalePreset     = ScalePreset::Chromatic; // pass-through, no quantization snap
    return p;
}

// -- Y axis routes to voices 0 and 1 ------------------------------------------

TEST_CASE("ChordEngine - Y axis drives voices 0 and 1", "[chord][axis][y]")
{
    ChordEngine::Params p = baseParams();

    SECTION("joystickY=-1.0: base=0, voice0=0, voice1=4")
    {
        p.joystickY = -1.0f;
        CHECK(ChordEngine::computePitch(0, p) == 0);
        CHECK(ChordEngine::computePitch(1, p) == 4);
    }

    SECTION("joystickY=+1.0: base=48, voice0=48, voice1=52")
    {
        p.joystickY = 1.0f;
        CHECK(ChordEngine::computePitch(0, p) == 48);
        CHECK(ChordEngine::computePitch(1, p) == 52);
    }

    SECTION("joystickY=0.0: base=24, voice0=24, voice1=28")
    {
        p.joystickY = 0.0f;
        CHECK(ChordEngine::computePitch(0, p) == 24);
        CHECK(ChordEngine::computePitch(1, p) == 28);
    }
}

// -- X axis routes to voices 2 and 3 ------------------------------------------

TEST_CASE("ChordEngine - X axis drives voices 2 and 3", "[chord][axis][x]")
{
    ChordEngine::Params p = baseParams();

    SECTION("joystickX=-1.0: base=0, voice2=7, voice3=10")
    {
        p.joystickX = -1.0f;
        CHECK(ChordEngine::computePitch(2, p) == 7);
        CHECK(ChordEngine::computePitch(3, p) == 10);
    }

    SECTION("joystickX=+1.0: base=48, voice2=55, voice3=58")
    {
        p.joystickX = 1.0f;
        CHECK(ChordEngine::computePitch(2, p) == 55);
        CHECK(ChordEngine::computePitch(3, p) == 58);
    }

    SECTION("joystickX=0.0: base=24, voice2=31, voice3=34")
    {
        p.joystickX = 0.0f;
        CHECK(ChordEngine::computePitch(2, p) == 31);
        CHECK(ChordEngine::computePitch(3, p) == 34);
    }
}

// -- Y and X axes are independent ---------------------------------------------

TEST_CASE("ChordEngine - axis independence: Y does not affect X voices", "[chord][axis][independence]")
{
    ChordEngine::Params p = baseParams();
    p.joystickX = -1.0f;  // X base=0
    p.joystickY = 1.0f;   // Y maxed -- must NOT bleed into voices 2/3

    CHECK(ChordEngine::computePitch(2, p) == 7);   // fifth above 0
    CHECK(ChordEngine::computePitch(3, p) == 10);  // tension above 0
}

TEST_CASE("ChordEngine - axis independence: X does not affect Y voices", "[chord][axis][independence]")
{
    ChordEngine::Params p = baseParams();
    p.joystickY = -1.0f;  // Y base=0
    p.joystickX = 1.0f;   // X maxed -- must NOT bleed into voices 0/1

    CHECK(ChordEngine::computePitch(0, p) == 0);  // root
    CHECK(ChordEngine::computePitch(1, p) == 4);  // third
}

// -- Global transpose ---------------------------------------------------------

TEST_CASE("ChordEngine - globalTranspose shifts all voices", "[chord][transpose]")
{
    ChordEngine::Params p = baseParams();
    p.joystickY = -1.0f;  // Y base=0
    p.joystickX = -1.0f;  // X base=0

    SECTION("+12 semitones: all voices shift up by 12")
    {
        p.globalTranspose = 12;
        CHECK(ChordEngine::computePitch(0, p) == 12);  // 0+0+12
        CHECK(ChordEngine::computePitch(1, p) == 16);  // 0+4+12
        CHECK(ChordEngine::computePitch(2, p) == 19);  // 0+7+12
        CHECK(ChordEngine::computePitch(3, p) == 22);  // 0+10+12
    }

    SECTION("-12 semitones: voices with small intervals clamp to 0")
    {
        p.globalTranspose = -12;
        CHECK(ChordEngine::computePitch(0, p) == 0);  // 0+0-12=-12 -> 0
        CHECK(ChordEngine::computePitch(1, p) == 0);  // 0+4-12=-8  -> 0
    }

    SECTION("+24 semitones with X max voice3: 48+10+24=82")
    {
        p.joystickX = 1.0f;  // X base=48
        p.globalTranspose = 24;
        CHECK(ChordEngine::computePitch(3, p) == 82);
    }
}

// -- Per-voice octave offsets -------------------------------------------------

TEST_CASE("ChordEngine - per-voice octave offsets", "[chord][octave]")
{
    ChordEngine::Params p = baseParams();
    p.joystickY = -1.0f;  // Y base=0
    p.joystickX = -1.0f;  // X base=0

    SECTION("octave +1 on voice0: 0+0+12=12")
    {
        p.octaves[0] = 1;
        CHECK(ChordEngine::computePitch(0, p) == 12);
    }

    SECTION("octave +2 on voice1: 0+4+24=28")
    {
        p.octaves[1] = 2;
        CHECK(ChordEngine::computePitch(1, p) == 28);
    }

    SECTION("octave -1 on voice1: 0+4-12=-8, clamped to 0")
    {
        p.octaves[1] = -1;
        CHECK(ChordEngine::computePitch(1, p) == 0);
    }

    SECTION("octave +3 on voice2 at X max: 48+7+36=91")
    {
        p.joystickX = 1.0f;
        p.octaves[2] = 3;
        CHECK(ChordEngine::computePitch(2, p) == 91);
    }
}

// -- MIDI output clamping -----------------------------------------------------

TEST_CASE("ChordEngine - output clamped to 0-127", "[chord][clamp]")
{
    ChordEngine::Params p = baseParams();

    SECTION("No underflow: large negative transpose pushes below 0")
    {
        p.joystickY      = -1.0f;  // base=0
        p.globalTranspose = -48;   // 0+0-48=-48 -> clamped to 0
        CHECK(ChordEngine::computePitch(0, p) == 0);
    }

    SECTION("No overflow: combined high values stay <=127")
    {
        // 48 (Y max) + 0 (root) + 24 (transpose) + 36 (octave+3) = 108
        p.joystickY      = 1.0f;
        p.globalTranspose = 24;
        p.octaves[0]     = 3;
        CHECK(ChordEngine::computePitch(0, p) == 108);
    }
}

// -- Scale quantization integration -------------------------------------------

TEST_CASE("ChordEngine - Major scale: all outputs on C major pitch classes", "[chord][quantize][major]")
{
    ChordEngine::Params p = baseParams();
    p.scalePreset    = ScalePreset::Major;
    p.useCustomScale = false;

    const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Major);
    const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Major);

    // Sweep Y axis; voices 0 and 1 must always output a C major pitch class
    for (float y = -1.0f; y <= 1.0f; y += 0.05f)
    {
        p.joystickY = y;
        for (int v = 0; v <= 1; ++v)
        {
            int pitch = ChordEngine::computePitch(v, p);
            int pc    = pitch % 12;
            bool inScale = false;
            for (int i = 0; i < size; ++i)
                if (pat[i] == pc) { inScale = true; break; }
            CHECK(inScale);
        }
    }
}

TEST_CASE("ChordEngine - custom single-note scale: all voices quantize to that note class", "[chord][quantize][custom]")
{
    ChordEngine::Params p = baseParams();
    p.useCustomScale = true;
    for (auto& n : p.customNotes) n = false;
    p.customNotes[0] = true;  // only C active

    // At Y bottom: voice0 raw=0 (C) and voice1 raw=4 (E) both snap to nearest C
    p.joystickY = -1.0f;
    CHECK(ChordEngine::computePitch(0, p) % 12 == 0);  // C
    CHECK(ChordEngine::computePitch(1, p) % 12 == 0);  // nearest C to E
}
