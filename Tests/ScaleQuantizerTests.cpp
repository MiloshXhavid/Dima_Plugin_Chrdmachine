#include <catch2/catch_test_macros.hpp>
#include "ScaleQuantizer.h"

// -- Preset pattern retrieval -------------------------------------------------

TEST_CASE("ScaleQuantizer preset patterns", "[scale][preset]")
{
    SECTION("Major has 7 notes: {0,2,4,5,7,9,11}")
    {
        CHECK(ScaleQuantizer::getScaleSize(ScalePreset::Major) == 7);
        const int* p = ScaleQuantizer::getScalePattern(ScalePreset::Major);
        CHECK(p[0] == 0);   // root
        CHECK(p[1] == 2);   // M2
        CHECK(p[2] == 4);   // M3
        CHECK(p[3] == 5);   // P4
        CHECK(p[4] == 7);   // P5
        CHECK(p[5] == 9);   // M6
        CHECK(p[6] == 11);  // M7
    }

    SECTION("Chromatic has 12 notes")
    {
        CHECK(ScaleQuantizer::getScaleSize(ScalePreset::Chromatic) == 12);
    }

    SECTION("Pentatonic Major has 5 notes")
    {
        CHECK(ScaleQuantizer::getScaleSize(ScalePreset::PentatonicMajor) == 5);
    }

    SECTION("Diminished (HW) has 8 notes: {0,1,3,4,6,7,9,10}")
    {
        CHECK(ScaleQuantizer::getScaleSize(ScalePreset::Diminished) == 8);
        const int* p = ScaleQuantizer::getScalePattern(ScalePreset::Diminished);
        CHECK(p[0] == 0);
        CHECK(p[1] == 1);
        CHECK(p[2] == 3);
        CHECK(p[3] == 4);
    }

    SECTION("All 20 presets return non-null non-empty names")
    {
        for (int i = 0; i < (int)ScalePreset::COUNT; ++i)
        {
            const char* name = ScaleQuantizer::getScaleName((ScalePreset)i);
            CHECK(name != nullptr);
            CHECK(name[0] != '\0');
        }
    }
}

// -- quantize() - in-scale notes pass through ---------------------------------

TEST_CASE("ScaleQuantizer::quantize - in-scale notes unchanged", "[quantize][in-scale]")
{
    const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Major);
    const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Major);
    // C major: {0,2,4,5,7,9,11}

    CHECK(ScaleQuantizer::quantize(60, pat, size) == 60); // C4
    CHECK(ScaleQuantizer::quantize(62, pat, size) == 62); // D4
    CHECK(ScaleQuantizer::quantize(64, pat, size) == 64); // E4
    CHECK(ScaleQuantizer::quantize(65, pat, size) == 65); // F4
    CHECK(ScaleQuantizer::quantize(67, pat, size) == 67); // G4
    CHECK(ScaleQuantizer::quantize(69, pat, size) == 69); // A4
    CHECK(ScaleQuantizer::quantize(71, pat, size) == 71); // B4
}

// -- quantize() - tie-breaking: ties go DOWN ----------------------------------

TEST_CASE("ScaleQuantizer::quantize - ties resolve to lower note", "[quantize][ties]")
{
    const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Major);
    const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Major);

    // C#4=61: C4(60) dist=1, D4(62) dist=1 -> tie -> 60 (lower wins)
    CHECK(ScaleQuantizer::quantize(61, pat, size) == 60);

    // Bb4=70: A4(69) dist=1, B4(71) dist=1 -> tie -> 69 (lower wins)
    CHECK(ScaleQuantizer::quantize(70, pat, size) == 69);

    // F#4=66: F4(65) dist=1, G4(67) dist=1 -> tie -> 65 (lower wins)
    CHECK(ScaleQuantizer::quantize(66, pat, size) == 65);

    // G#4=68: G4(67) dist=1, A4(69) dist=1 -> tie -> 67 (lower wins)
    CHECK(ScaleQuantizer::quantize(68, pat, size) == 67);

    // D#4=63: D4(62) dist=1, E4(64) dist=1 -> tie -> 62 (lower wins)
    CHECK(ScaleQuantizer::quantize(63, pat, size) == 62);
}

// -- quantize() - MIDI boundary values ----------------------------------------

TEST_CASE("ScaleQuantizer::quantize - MIDI boundary values", "[quantize][boundary]")
{
    const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Major);
    const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Major);

    SECTION("MIDI 0 (C-2) is in C major and passes through")
    {
        // 0 % 12 = 0 (C) -- C is in C major
        CHECK(ScaleQuantizer::quantize(0, pat, size) == 0);
    }

    SECTION("MIDI 127 (G9) is in C major (127 % 12 = 7 = G) and passes through")
    {
        CHECK(ScaleQuantizer::quantize(127, pat, size) == 127);
    }
}

// -- quantize() - chromatic scale (all notes pass through) --------------------

TEST_CASE("ScaleQuantizer::quantize - chromatic passes all 128 notes unchanged", "[quantize][chromatic]")
{
    const int* pat  = ScaleQuantizer::getScalePattern(ScalePreset::Chromatic);
    const int  size = ScaleQuantizer::getScaleSize(ScalePreset::Chromatic);

    for (int i = 0; i <= 127; ++i)
        CHECK(ScaleQuantizer::quantize(i, pat, size) == i);
}

// -- buildCustomPattern -------------------------------------------------------

TEST_CASE("ScaleQuantizer::buildCustomPattern", "[custom]")
{
    SECTION("Only root active - size 1, pattern[0]=0")
    {
        bool flags[12] = {true, false, false, false, false, false,
                          false, false, false, false, false, false};
        int pattern[12]; int size = 0;
        ScaleQuantizer::buildCustomPattern(flags, pattern, size);
        CHECK(size == 1);
        CHECK(pattern[0] == 0);
    }

    SECTION("All flags false - fallback to root (size 1, pattern[0]=0)")
    {
        bool flags[12] = {};
        int pattern[12]; int size = 0;
        ScaleQuantizer::buildCustomPattern(flags, pattern, size);
        CHECK(size == 1);
        CHECK(pattern[0] == 0);
    }

    SECTION("All 12 flags true - chromatic equivalent (size 12, pattern[i]=i)")
    {
        bool flags[12];
        for (auto& f : flags) f = true;
        int pattern[12]; int size = 0;
        ScaleQuantizer::buildCustomPattern(flags, pattern, size);
        CHECK(size == 12);
        for (int i = 0; i < 12; ++i)
            CHECK(pattern[i] == i);
    }

    SECTION("Minor triad {0,3,7} - size 3, correct semitones")
    {
        bool flags[12] = {true, false, false, true, false, false,
                          false, true, false, false, false, false};
        int pattern[12]; int size = 0;
        ScaleQuantizer::buildCustomPattern(flags, pattern, size);
        CHECK(size == 3);
        CHECK(pattern[0] == 0);
        CHECK(pattern[1] == 3);
        CHECK(pattern[2] == 7);
    }
}
