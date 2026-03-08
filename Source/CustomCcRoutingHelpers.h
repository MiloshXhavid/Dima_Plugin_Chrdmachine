#pragma once

// CustomCcRoutingHelpers.h
// Standalone header — no JUCE dependency. Safe to include from Catch2 test targets.
//
// Provides ccDestToNumber() and resolveFilterCcMode() as inline functions so that
// the same CC dispatch logic used in processBlock() can be unit-tested independently.

#include <algorithm> // std::min / std::max

// ─── Constants ────────────────────────────────────────────────────────────────

// Number of LFO/Gate target modes at the low indices of filterXMode / filterYMode.
// Indices 0-6: LFO-X/Y Freq, Phase, Level, Gate, cross-axis Freq, Phase, Level.
static constexpr int kNumLfoTargetModes = 7;

// Named CC numbers for filterXMode / filterYMode indices 7-24.
// Index into this array = (mode - kNumLfoTargetModes).
static constexpr int kFilterCcNums[18] = {
    1, 2, 5, 7, 10, 11, 12, 16, 17,       // CC1..CC17
    71, 72, 73, 74, 75, 76, 77, 91, 93     // CC71..CC93
};

// Number of named CC entries in kFilterCcNums.
static constexpr int kNumFilterCcModes = 18;

// Index of the "Custom CC..." entry in lfoXCcDest / lfoYCcDest AudioParameterChoice.
// Indices 0-18: named entries (Off + 18 named CCs).  Index 19: custom.
static constexpr int kCustomCcDestIdx = 19;

// Index of the "Custom CC..." entry in filterXMode / filterYMode AudioParameterChoice.
// Indices 0-6: LFO/Gate targets.  Indices 7-24: 18 named CCs.  Index 25: custom.
// kNumLfoTargetModes(7) + kNumFilterCcModes(18) = 25.
static constexpr int kCustomFilterModeIdx = 25;

// Lookup table for ccDestToNumber — 19 entries, indices 0-18.
// Index 0 = Off (-1), indices 1-18 = named CC numbers.
static constexpr int kCcDestTable[19] = {
    -1, 1, 2, 5, 7, 10, 11, 12, 16, 17,
    71, 72, 73, 74, 75, 76, 77, 91, 93
};

// ─── Clamp helper (avoids JUCE dependency) ────────────────────────────────────

static inline int ccClamp(int v, int lo, int hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// ─── ccDestToNumber ───────────────────────────────────────────────────────────
//
// Maps an lfoXCcDest / lfoYCcDest AudioParameterChoice index to a MIDI CC number.
//
//   idx 0-18  : named entries — returns kCcDestTable[idx]  (-1 = Off)
//   idx 19    : "Custom CC..." — returns customCcVal clamped to [0, 127]
//   other     : -1  (out of range)
//
// customCcVal defaults to 74 (Filter Cutoff) — a reasonable fallback.

inline int ccDestToNumber(int idx, int customCcVal = 74)
{
    if (idx == kCustomCcDestIdx)
        return ccClamp(customCcVal, 0, 127);
    if (idx >= 0 && idx < 19)
        return kCcDestTable[idx];
    return -1;
}

// ─── resolveFilterCcMode ──────────────────────────────────────────────────────
//
// Maps a filterXMode / filterYMode AudioParameterChoice index to a MIDI CC number.
//
//   mode 0-6  : LFO/Gate targets — returns -1  (not a CC mode)
//   mode 7-24 : named CC modes   — returns kFilterCcNums[mode - kNumLfoTargetModes]
//   mode 25   : "Custom CC..."   — returns customCcVal clamped to [0, 127]
//   other     : -1  (out of range)
//
// customCcVal defaults to 74 (Filter Cutoff) — a reasonable fallback.

inline int resolveFilterCcMode(int mode, int customCcVal = 74)
{
    if (mode == kCustomFilterModeIdx)
        return ccClamp(customCcVal, 0, 127);
    if (mode >= kNumLfoTargetModes && mode < kCustomFilterModeIdx)
        return kFilterCcNums[mode - kNumLfoTargetModes];
    return -1;
}
