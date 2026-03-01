#pragma once
#include <string>
#include <algorithm>

// ─── Chord name helper ───────────────────────────────────────────────────────
// Given 4 MIDI pitches, returns a jazz chord name string (e.g. "Cmaj7",
// "G7#9", "Fm9").
//
// Root is ALWAYS voice 0 (pitches[0]).  Octave differences between voices are
// ignored — all notes are reduced to pitch classes and interpreted as intervals
// above the root.  This means the display tracks wherever the joystick sends
// voice 0, regardless of the global transpose key selector.

inline std::string computeChordNameStr(const int pitches[4])
{
    static const char* kN[12] = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};

    // Root = voice 0, always.
    const int rootPc = ((pitches[0] % 12) + 12) % 12;

    // Collect unique pitch classes; root is first and never duplicated.
    int pcs[4]; int nPcs = 1;
    pcs[0] = rootPc;
    for (int i = 1; i < 4; ++i)
    {
        const int pc = ((pitches[i] % 12) + 12) % 12;
        bool dup = false;
        for (int j = 0; j < nPcs; ++j) if (pcs[j] == pc) { dup = true; break; }
        if (!dup && nPcs < 4) pcs[nPcs++] = pc;
    }

    // Build interval array relative to root, sorted ascending.
    // ivs[0] is always 0 (the root itself).
    int ivs[4];
    ivs[0] = 0;
    for (int i = 1; i < nPcs; ++i)
        ivs[i] = (pcs[i] - rootPc + 12) % 12;
    for (int i = 1; i < nPcs - 1; ++i)
        for (int j = i + 1; j < nPcs; ++j)
            if (ivs[i] > ivs[j]) std::swap(ivs[i], ivs[j]);

    // ─── Chord template table ────────────────────────────────────────────────
    // Sorted intervals from root (ivs[0]=0 always).  -1 = unused slot.
    // 4-note chords first (most specific); jazz extensions before standard so
    // extended names win when both would match.
    static const struct { int ivs[4]; const char* nm; } kT[] = {
        // ── 4-note jazz extensions ────────────────────────────────────────────
        {{0, 2, 4,11}, "maj9"},      // C E B D  — major 9th (no 5th, jazz voicing)
        {{0, 2, 3,11}, "mmaj9"},     // C Eb B D — minor major 9th
        {{0, 2, 4,10}, "9"},         // C E Bb D — dominant 9th
        {{0, 2, 3,10}, "m9"},        // C Eb Bb D — minor 9th
        {{0, 1, 4,10}, "7b9"},       // C E Bb Db — dominant flat 9
        {{0, 3, 4,10}, "7#9"},       // C E Bb Eb — dominant sharp 9 (Hendrix)
        {{0, 4, 6,10}, "7#11"},      // C E F# Bb — Lydian dominant
        {{0, 4, 6,11}, "maj7#11"},   // C E F# B — Lydian major 7
        {{0, 4, 9,11}, "maj13"},     // C E B A  — major 13th
        {{0, 4, 8,11}, "maj7#5"},    // C E Ab B — augmented major 7th
        {{0, 3, 7,11}, "mmaj7"},     // C Eb G B — minor major 7th
        {{0, 3, 9,10}, "m13"},       // C Eb Bb A — minor 13th
        {{0, 3, 5,10}, "m11"},       // C Eb F Bb — minor 11th
        {{0, 4, 9,10}, "13"},        // C E Bb A — dominant 13th
        // ── 4-note standard chords ────────────────────────────────────────────
        {{0, 4, 7,11}, "maj7"},
        {{0, 3, 7,10}, "m7"},
        {{0, 4, 7,10}, "7"},
        {{0, 3, 6,10}, "m7b5"},
        {{0, 3, 6, 9}, "dim7"},
        {{0, 4, 8,10}, "7#5"},
        {{0, 4, 7, 9}, "6"},
        {{0, 3, 7, 9}, "m6"},
        {{0, 2, 4, 7}, "add9"},
        {{0, 2, 3, 7}, "madd9"},
        {{0, 5, 7,10}, "7sus4"},
        {{0, 2, 7,10}, "9sus4"},
        {{0, 4, 5, 7}, "add11"},
        // ── 3-note triads ─────────────────────────────────────────────────────
        {{0, 4, 7,-1}, "maj"},
        {{0, 3, 7,-1}, "m"},
        {{0, 3, 6,-1}, "dim"},
        {{0, 4, 8,-1}, "aug"},
        {{0, 2, 7,-1}, "sus2"},
        {{0, 5, 7,-1}, "sus4"},
        // ── 2-note intervals ──────────────────────────────────────────────────
        {{0, 1,-1,-1}, "b2"},
        {{0, 2,-1,-1}, "2"},
        {{0, 3,-1,-1}, "m3"},
        {{0, 4,-1,-1}, "3"},
        {{0, 5,-1,-1}, "4"},
        {{0, 6,-1,-1}, "#4"},
        {{0, 7,-1,-1}, "5"},
        {{0, 8,-1,-1}, "#5"},
        {{0, 9,-1,-1}, "6"},
        {{0,10,-1,-1}, "7"},
        {{0,11,-1,-1}, "M7"},
    };
    constexpr int kNumT = static_cast<int>(sizeof(kT) / sizeof(kT[0]));

    // Template match: note count must equal template length exactly.
    for (int ti = 0; ti < kNumT; ++ti)
    {
        int tLen = 0;
        while (tLen < 4 && kT[ti].ivs[tLen] >= 0) ++tLen;
        if (nPcs != tLen) continue;

        bool match = true;
        for (int i = 0; i < tLen; ++i)
            if (ivs[i] != kT[ti].ivs[i]) { match = false; break; }
        if (match)
            return std::string(kN[rootPc]) + std::string(kT[ti].nm);
    }

    // Fallback for 4 unique notes: drop one non-root note at a time (highest
    // interval first) and retry against 3-note templates.
    if (nPcs == 4)
    {
        for (int skip = 3; skip >= 1; --skip)
        {
            int sub[3]; int si = 0;
            for (int i = 0; i < 4; ++i)
                if (i != skip) sub[si++] = ivs[i];

            for (int ti = 0; ti < kNumT; ++ti)
            {
                int tLen = 0;
                while (tLen < 4 && kT[ti].ivs[tLen] >= 0) ++tLen;
                if (tLen != 3) continue;

                if (sub[0] == kT[ti].ivs[0] &&
                    sub[1] == kT[ti].ivs[1] &&
                    sub[2] == kT[ti].ivs[2])
                    return std::string(kN[rootPc]) + std::string(kT[ti].nm);
            }
        }
    }

    // Fallback for 3 unique notes: drop one non-root note and retry against
    // 2-note interval templates.  Try keeping the highest interval first (most
    // harmonically defining), then the lower one.  Since all intervals 1-11 are
    // covered by the 2-note table this always produces a result.
    if (nPcs >= 3)
    {
        for (int skip = 1; skip <= 2; ++skip)
        {
            const int iv2 = ivs[3 - skip]; // skip=1→ivs[2] (highest), skip=2→ivs[1]
            for (int ti = 0; ti < kNumT; ++ti)
            {
                int tLen = 0;
                while (tLen < 4 && kT[ti].ivs[tLen] >= 0) ++tLen;
                if (tLen != 2) continue;
                if (iv2 == kT[ti].ivs[1])
                    return std::string(kN[rootPc]) + std::string(kT[ti].nm);
            }
        }
    }

    // Last resort: root note name only (only reached when all 4 voices are
    // the same pitch class).
    return std::string(kN[rootPc]);
}
