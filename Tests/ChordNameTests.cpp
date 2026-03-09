#include <catch2/catch_test_macros.hpp>
#include "ChordNameHelper.h"

// Root is always voice 0 (pitches[0]).  Octave differences are ignored.
static std::string cn(int p0, int p1, int p2, int p3)
{
    const int p[4] = {p0, p1, p2, p3};
    return computeChordNameStr(p);
}

// Helper for smart variant — ctx built inline per test
static std::string cnSmart(int p0, int p1, int p2, int p3,
                            uint8_t mask, const int* pat, int sz)
{
    const int p[4] = {p0, p1, p2, p3};
    ChordNameContext ctx { pat, sz, mask };
    return computeChordNameSmart(p, ctx);
}

// ── 1-note (all voices same pitch class) ─────────────────────────────────────

TEST_CASE("ChordName - 1 unique note returns root name", "[chordname][1note]")
{
    CHECK(cn(60, 60, 60, 60) == "C");
    CHECK(cn(64, 64, 64, 64) == "E");
    CHECK(cn(66, 66, 66, 66) == "F#");
}

// ── 2-note: voice 0 always the root ──────────────────────────────────────────

TEST_CASE("ChordName - 2-note minor 2nd (b2)", "[chordname][2note]")
{
    CHECK(cn(60, 61, 60, 61) == "Cb2");
    CHECK(cn(67, 68, 67, 68) == "Gb2");
}

TEST_CASE("ChordName - 2-note major 2nd (2)", "[chordname][2note]")
{
    CHECK(cn(60, 62, 60, 62) == "C2");
    CHECK(cn(69, 71, 69, 71) == "A2");
}

TEST_CASE("ChordName - 2-note minor 3rd (m3)", "[chordname][2note]")
{
    CHECK(cn(60, 63, 60, 63) == "Cm3");
    CHECK(cn(64, 67, 64, 67) == "Em3");
}

TEST_CASE("ChordName - 2-note major 3rd (3)", "[chordname][2note]")
{
    CHECK(cn(60, 64, 60, 64) == "C3");
    CHECK(cn(65, 69, 65, 69) == "F3");
}

TEST_CASE("ChordName - 2-note perfect 4th (4)", "[chordname][2note]")
{
    CHECK(cn(60, 65, 60, 65) == "C4");
    CHECK(cn(71, 76, 71, 76) == "B4");
}

TEST_CASE("ChordName - 2-note tritone (#4)", "[chordname][2note]")
{
    CHECK(cn(60, 66, 60, 66) == "C#4");
    CHECK(cn(62, 68, 62, 68) == "D#4");
}

TEST_CASE("ChordName - 2-note perfect 5th (power chord, 5)", "[chordname][2note]")
{
    CHECK(cn(60, 67, 60, 67) == "C5");
    CHECK(cn(69, 76, 69, 76) == "A5");
}

TEST_CASE("ChordName - 2-note augmented 5th (#5)", "[chordname][2note]")
{
    CHECK(cn(60, 68, 60, 68) == "C#5");
    CHECK(cn(64, 72, 64, 72) == "E#5");
}

TEST_CASE("ChordName - 2-note major 6th (6)", "[chordname][2note]")
{
    CHECK(cn(60, 69, 60, 69) == "C6");
    CHECK(cn(67, 76, 67, 76) == "G6");
}

TEST_CASE("ChordName - 2-note minor 7th (7)", "[chordname][2note]")
{
    CHECK(cn(60, 70, 60, 70) == "C7");
    CHECK(cn(62, 72, 62, 72) == "D7");
}

TEST_CASE("ChordName - 2-note major 7th (M7)", "[chordname][2note]")
{
    CHECK(cn(60, 71, 60, 71) == "CM7");
    CHECK(cn(65, 76, 65, 76) == "FM7");
}

// ── 3-note triads ─────────────────────────────────────────────────────────────

TEST_CASE("ChordName - major triad", "[chordname][3note]")
{
    CHECK(cn(60, 64, 67, 67) == "Cmaj");
    CHECK(cn(69, 73, 76, 76) == "Amaj");
}

TEST_CASE("ChordName - minor triad", "[chordname][3note]")
{
    CHECK(cn(60, 63, 67, 67) == "Cm");
    CHECK(cn(69, 72, 76, 76) == "Am");
}

TEST_CASE("ChordName - diminished triad", "[chordname][3note]")
{
    CHECK(cn(60, 63, 66, 66) == "Cdim");
}

TEST_CASE("ChordName - augmented triad", "[chordname][3note]")
{
    CHECK(cn(60, 64, 68, 68) == "Caug");
}

TEST_CASE("ChordName - sus2 triad", "[chordname][3note]")
{
    CHECK(cn(60, 62, 67, 67) == "Csus2");
}

TEST_CASE("ChordName - sus4 triad", "[chordname][3note]")
{
    CHECK(cn(60, 65, 67, 67) == "Csus4");
}

// ── 4-note standard chords ────────────────────────────────────────────────────

TEST_CASE("ChordName - maj7", "[chordname][4note]")
{
    CHECK(cn(60, 64, 67, 71) == "Cmaj7");
}

TEST_CASE("ChordName - m7", "[chordname][4note]")
{
    CHECK(cn(60, 63, 67, 70) == "Cm7");
}

TEST_CASE("ChordName - dominant 7", "[chordname][4note]")
{
    CHECK(cn(60, 64, 67, 70) == "C7");
}

TEST_CASE("ChordName - dim7", "[chordname][4note]")
{
    CHECK(cn(60, 63, 66, 69) == "Cdim7");
}

TEST_CASE("ChordName - m7b5 (half-dim)", "[chordname][4note]")
{
    CHECK(cn(60, 63, 66, 70) == "Cm7b5");
}

TEST_CASE("ChordName - 6th chord", "[chordname][4note]")
{
    CHECK(cn(60, 64, 67, 69) == "C6");   // C E G A
}

TEST_CASE("ChordName - minor 6th chord", "[chordname][4note]")
{
    CHECK(cn(60, 63, 67, 69) == "Cm6");  // C Eb G A
}

// ── 4-note jazz extensions ────────────────────────────────────────────────────

TEST_CASE("ChordName - dominant 9th", "[chordname][jazz]")
{
    // C E Bb D (no 5th — jazz voicing)
    CHECK(cn(60, 64, 70, 62) == "C9");
}

TEST_CASE("ChordName - major 9th", "[chordname][jazz]")
{
    // C E B D
    CHECK(cn(60, 64, 71, 62) == "Cmaj9");
}

TEST_CASE("ChordName - minor 9th", "[chordname][jazz]")
{
    // C Eb Bb D
    CHECK(cn(60, 63, 70, 62) == "Cm9");
}

TEST_CASE("ChordName - dominant 7 flat 9", "[chordname][jazz]")
{
    // C E Bb Db
    CHECK(cn(60, 64, 70, 61) == "C7b9");
}

TEST_CASE("ChordName - dominant 7 sharp 9 (Hendrix)", "[chordname][jazz]")
{
    // C E Bb Eb (minor 3rd written as #9)
    CHECK(cn(60, 64, 70, 63) == "C7#9");
}

TEST_CASE("ChordName - Lydian dominant (7#11)", "[chordname][jazz]")
{
    // C E F# Bb
    CHECK(cn(60, 64, 66, 70) == "C7#11");
}

TEST_CASE("ChordName - Lydian major 7 (maj7#11)", "[chordname][jazz]")
{
    // C E F# B
    CHECK(cn(60, 64, 66, 71) == "Cmaj7#11");
}

TEST_CASE("ChordName - minor major 7th", "[chordname][jazz]")
{
    // C Eb G B
    CHECK(cn(60, 63, 67, 71) == "Cmmaj7");
}

TEST_CASE("ChordName - dominant 13th", "[chordname][jazz]")
{
    // C E Bb A
    CHECK(cn(60, 64, 70, 69) == "C13");
}

TEST_CASE("ChordName - minor 11th", "[chordname][jazz]")
{
    // C Eb F Bb
    CHECK(cn(60, 63, 65, 70) == "Cm11");
}

TEST_CASE("ChordName - 7sus4", "[chordname][jazz]")
{
    // C F G Bb
    CHECK(cn(60, 65, 67, 70) == "C7sus4");
}

// ── Root always = voice 0 ─────────────────────────────────────────────────────

TEST_CASE("ChordName - voice 0 is root regardless of pitch ordering", "[chordname][root]")
{
    // Voice 0 = A, voices 1-2 = C E (pitch classes above A: m3, P5) → Am
    CHECK(cn(69, 72, 76, 76) == "Am");

    // Voice 0 = A4, voice 1 = C5, voice 2 = E5, voice 3 = G5 → Am7
    CHECK(cn(69, 72, 76, 79) == "Am7");

    // Voice 0 = G, voices = B D F → G7
    CHECK(cn(67, 71, 74, 65) == "G7");
}

TEST_CASE("ChordName - voice 0 root holds even when another note is lower in octave", "[chordname][root]")
{
    // Voice 0 = C5 (72), voice 1 = E3 (52), voice 2 = G3 (55), voice 3 = Bb2 (46)
    // Pitch classes: C=0, E=4, G=7, Bb=10 → C7
    CHECK(cn(72, 52, 55, 46) == "C7");
}

// ── Octave duplicates collapse to pitch class ─────────────────────────────────

TEST_CASE("ChordName - octave duplicates collapse to pitch class", "[chordname][octave]")
{
    // C3 + E4 + G3 + C5: three unique pcs → major triad
    CHECK(cn(48, 64, 55, 72) == "Cmaj");
    // C + G (power chord) across different octaves
    CHECK(cn(36, 67, 48, 79) == "C5");
}

// ── Smart chord inference (computeChordNameSmart) ─────────────────────────────

// Scale patterns used across tests (inline, no ScaleQuantizer dependency)
static const int kMajor[]  = {0, 2, 4, 5, 7, 9, 11};  static const int kMajorSz  = 7;
static const int kMinor[]  = {0, 2, 3, 5, 7, 8, 10};  static const int kMinorSz  = 7;
static const int kMixo[]   = {0, 2, 4, 5, 7, 9, 10};  static const int kMixoSz   = 7;

TEST_CASE("ChordName smart - mask=0xF returns same as original (Cmaj7)", "[chordname][smart]")
{
    // Test A: all voices real — smart must equal dumb for Cmaj7 (60,64,67,71)
    CHECK(cnSmart(60, 64, 67, 71, 0xF, kMajor, kMajorSz) == cn(60, 64, 67, 71));
}

TEST_CASE("ChordName smart - major scale voice1 absent infers major third", "[chordname][smart]")
{
    // Test B: mask=0xD=1101 → voice1 bit NOT set; root=C(60), voice2=G(67), voice3=B(71)
    // Major scale has major third (E=4) only → infers E → {C,E,G,B} = C△ (maj7)
    CHECK(cnSmart(60, 64, 67, 71, 0xD, kMajor, kMajorSz) == "Cmaj7");
}

TEST_CASE("ChordName smart - natural minor scale voice1 absent infers minor third", "[chordname][smart]")
{
    // Test C: mask=0xD, root=C(60), voice2=G(67), voice3=Bb(70)
    // Minor scale has minor third (Eb=3) only → infers Eb → {C,Eb,G,Bb} = Cm7
    CHECK(cnSmart(60, 63, 67, 70, 0xD, kMinor, kMinorSz) == "Cm7");
}

TEST_CASE("ChordName smart - major scale voice3 absent infers maj7 (Unicode triangle)", "[chordname][smart]")
{
    // Test D: mask=0x7=0111 → voice3 NOT set; root=C, voice1=E(64), voice2=G(67)
    // Major scale has maj7 (B=11) only → infers B → {C,E,G,B} = C△
    CHECK(cnSmart(60, 64, 67, 71, 0x7, kMajor, kMajorSz) == "Cmaj7");
}

TEST_CASE("ChordName smart - mixolydian scale voice3 absent infers dominant 7", "[chordname][smart]")
{
    // Test E: mask=0x7, root=C, voice1=E(64), voice2=G(67)
    // Mixolydian has dom7 (Bb=10) but NOT maj7 → infers Bb → {C,E,G,Bb} = C7
    CHECK(cnSmart(60, 64, 67, 70, 0x7, kMixo, kMixoSz) == "C7");
}

TEST_CASE("ChordName smart - mask=0xF no inference applied matches original", "[chordname][smart]")
{
    // Test F: no inference when all voices real
    CHECK(cnSmart(60, 63, 67, 70, 0xF, kMinor, kMinorSz) == cn(60, 63, 67, 70));
}

TEST_CASE("ChordName smart - non-C root major scale voice1 absent infers major quality", "[chordname][smart]")
{
    // Test G: root=G(67), voices 0/2/3 = G/D(74)/F#(78)=F#; voice1=B(71) real but masked
    // rootPc=7; major third of G = (7+4)%12=11=B; minor third = (7+3)%12=10=Bb
    // Major scale has B (interval 4) but not Bb (interval 3) → infer B
    // Effective: {G,B,D,F#} → intervals from G: 0,4,7,11 → G△ (Gmaj7)
    CHECK(cnSmart(67, 71, 74, 78, 0xD, kMajor, kMajorSz) == "Gmaj7");
}
