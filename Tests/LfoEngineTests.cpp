#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "LfoEngine.h"
#include <cmath>

// ─── Test helper ──────────────────────────────────────────────────────────────
// Default ProcessParams in sync mode for deterministic phase control.
// Callers override only the fields relevant to their test case.
static ProcessParams baseParams()
{
    ProcessParams p;
    p.sampleRate    = 44100.0;
    p.blockSize     = 512;
    p.bpm           = 120.0;
    p.syncMode      = true;          // sync mode → phase derived from ppqPosition
    p.isDawPlaying  = true;
    p.ppqPosition   = 0.0;
    p.subdivBeats   = 4.0;           // 1-bar cycle (4 quarter-note beats)
    p.maxCycleBeats = 16.0;
    p.waveform      = Waveform::Sine;
    p.phaseShift    = 0.0f;
    p.distortion    = 0.0f;
    p.level         = 1.0f;
    return p;
}

// ─── Waveform tests ───────────────────────────────────────────────────────────

TEST_CASE("LfoEngine sine at phase 0.25 produces +1", "[lfo][sine]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform    = Waveform::Sine;
    p.ppqPosition = 1.0;   // 1.0 / 4.0 beats = 0.25 normalized phase → sin(pi/2) = +1
    CHECK(std::abs(lfo.process(p) - 1.0f) < 0.001f);
}

TEST_CASE("LfoEngine triangle waveform four quarter-cycle samples", "[lfo][triangle]")
{
    // Formula: (phi < 0.5) ? (4*phi - 1) : (3 - 4*phi)
    // phi=0.00 → 4*0.00-1 = -1.0  (start, trough)
    // phi=0.25 → 4*0.25-1 =  0.0  (midpoint of rising half)
    // phi=0.50 → 3-4*0.50 = +1.0  (peak)
    // phi=0.75 → 3-4*0.75 =  0.0  (midpoint of falling half)
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform    = Waveform::Triangle;
    p.subdivBeats = 4.0;

    p.ppqPosition = 0.0;   // phi=0.00 → -1
    CHECK(lfo.process(p) == Catch::Approx(-1.0f).margin(0.001f));

    lfo.reset(); p.ppqPosition = 1.0;  // phi=0.25 → 0.0
    CHECK(lfo.process(p) == Catch::Approx(0.0f).margin(0.001f));

    lfo.reset(); p.ppqPosition = 2.0;  // phi=0.50 → +1
    CHECK(lfo.process(p) == Catch::Approx(1.0f).margin(0.001f));

    lfo.reset(); p.ppqPosition = 3.0;  // phi=0.75 → 0.0
    CHECK(lfo.process(p) == Catch::Approx(0.0f).margin(0.001f));
}

TEST_CASE("LfoEngine SawUp waveform range", "[lfo][sawup]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform = Waveform::SawUp;

    p.ppqPosition = 0.0;   // phi=0   → 2*0-1 = -1
    CHECK(lfo.process(p) == Catch::Approx(-1.0f).margin(0.001f));

    lfo.reset(); p.ppqPosition = 2.0;   // phi=0.5 → 2*0.5-1 = 0
    CHECK(std::abs(lfo.process(p)) < 0.001f);
}

TEST_CASE("LfoEngine SawDown waveform range", "[lfo][sawdown]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform = Waveform::SawDown;

    p.ppqPosition = 0.0;   // phi=0   → 1-2*0 = +1
    CHECK(lfo.process(p) == Catch::Approx(1.0f).margin(0.001f));

    lfo.reset(); p.ppqPosition = 2.0;   // phi=0.5 → 1-2*0.5 = 0
    CHECK(std::abs(lfo.process(p)) < 0.001f);
}

TEST_CASE("LfoEngine Square waveform high and low", "[lfo][square]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform = Waveform::Square;

    p.ppqPosition = 0.4;   // phi = 0.4/4.0 = 0.1 → (0.1 < 0.5) → +1
    CHECK(lfo.process(p) == 1.0f);

    lfo.reset(); p.ppqPosition = 2.4;   // phi = 2.4/4.0 = 0.6 → (0.6 >= 0.5) → -1
    CHECK(lfo.process(p) == -1.0f);
}

// ─── S&H tests ────────────────────────────────────────────────────────────────

TEST_CASE("LfoEngine S&H holds same value for full cycle", "[lfo][sh]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform    = Waveform::SH;
    p.subdivBeats = 4.0;

    p.ppqPosition = 0.0;
    const float held = lfo.process(p);
    // All within cycle 0 (ppqPos < 4.0): same held value
    p.ppqPosition = 1.0; CHECK(lfo.process(p) == held);
    p.ppqPosition = 2.0; CHECK(lfo.process(p) == held);
    p.ppqPosition = 3.9; CHECK(lfo.process(p) == held);
}

TEST_CASE("LfoEngine S&H generates new value at cycle boundary", "[lfo][sh]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform    = Waveform::SH;
    p.subdivBeats = 4.0;

    p.ppqPosition = 3.9;   // cycle index 0
    const float firstCycle = lfo.process(p);

    p.ppqPosition = 4.1;   // cycle index 1 — boundary crossed
    const float secondCycle = lfo.process(p);

    // LCG produces distinct values; collision astronomically unlikely
    CHECK(firstCycle != secondCycle);
}

TEST_CASE("LfoEngine S&H prevCycle sentinel fires on first process call", "[lfo][sh]")
{
    LfoEngine lfo;   // fresh instance: prevCycle_ = -1
    auto p = baseParams();
    p.waveform    = Waveform::SH;
    p.ppqPosition = 0.0;   // cycle index 0 != -1 → LCG fires

    // shHeld_ is updated from LCG on first call; result must be in [-1, +1]
    const float result = lfo.process(p);
    CHECK(result >= -1.0f);
    CHECK(result <= 1.0f);
}

// ─── Random waveform tests ────────────────────────────────────────────────────

TEST_CASE("LfoEngine Random waveform ignores rate and stays in [-1, 1]", "[lfo][random]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform = Waveform::Random;
    p.syncMode = false;

    p.rateHz = 0.01f;
    for (int i = 0; i < 50; ++i)
    {
        const float v = lfo.process(p);
        CHECK(v >= -1.0f);
        CHECK(v <= 1.0f);
    }

    p.rateHz = 20.0f;
    for (int i = 0; i < 50; ++i)
    {
        const float v = lfo.process(p);
        CHECK(v >= -1.0f);
        CHECK(v <= 1.0f);
    }
}

// ─── Distortion and level tests ───────────────────────────────────────────────

TEST_CASE("LfoEngine distortion=0 produces exact waveform output", "[lfo][distortion]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform   = Waveform::Square;
    p.distortion = 0.0f;
    p.ppqPosition = 0.4;   // phi=0.1 → Square = +1.0 (exact, no noise)

    CHECK(lfo.process(p) == 1.0f);
}

TEST_CASE("LfoEngine level scales output", "[lfo][level]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform    = Waveform::Square;
    p.level       = 0.5f;
    p.ppqPosition = 0.4;   // phi=0.1 → Square = +1.0 → scaled to +0.5

    CHECK(lfo.process(p) == Catch::Approx(0.5f).margin(0.001f));
}

TEST_CASE("LfoEngine phaseShift=0.5 inverts Square output", "[lfo][phaseshift]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform    = Waveform::Square;
    p.phaseShift  = 0.5f;   // phi 0.1 + 0.5 = 0.6 → Square = -1.0
    p.ppqPosition = 0.4;    // phi=0.1 before shift

    CHECK(lfo.process(p) == -1.0f);
}

// ─── Free mode tests ──────────────────────────────────────────────────────────

TEST_CASE("LfoEngine free mode accumulates phase without drift", "[lfo][free][perf03]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.syncMode     = false;
    p.isDawPlaying = false;
    p.rateHz       = 1.0f;      // 1 Hz = 1 cycle per second
    p.sampleRate   = 1000.0;    // 1000 samples per second → 1000-sample cycle
    p.blockSize    = 100;        // 10 blocks per full cycle
    p.waveform     = Waveform::SawUp;
    p.level        = 1.0f;

    // Branch C increments phase THEN returns it, so:
    //   Call 1: phase_ becomes 0.1, SawUp(0.1)=-0.8
    //   Call 2: phase_ becomes 0.2, SawUp(0.2)=-0.6
    //   Call 3: phase_ becomes 0.3, SawUp(0.3)=-0.4
    //   Call 4: phase_ becomes 0.4, SawUp(0.4)=-0.2
    //   Call 5: phase_ becomes 0.5, SawUp(0.5)=0.0  ← zero crossing
    // Run 4 warm-up blocks, then confirm 5th call produces near-zero output
    for (int i = 0; i < 4; ++i)
        lfo.process(p);

    // 5th call: phase_ goes from 0.4 to 0.5, SawUp(0.5) = 2*0.5-1 = 0.0
    const float out = lfo.process(p);
    CHECK(std::abs(out) < 0.001f);
}

// ─── Structural / PERF-02 test ────────────────────────────────────────────────

TEST_CASE("LfoEngine process returns float without side effects", "[lfo][perf02]")
{
    // Two independent instances with identical params produce identical output.
    // Confirms no shared global mutable state between instances.
    LfoEngine lfo1, lfo2;
    auto p = baseParams();
    p.waveform    = Waveform::Sine;
    p.ppqPosition = 1.0;   // deterministic phase 0.25

    const float out1 = lfo1.process(p);
    const float out2 = lfo2.process(p);

    CHECK(out1 == out2);
}

// ─── LCG range verification ───────────────────────────────────────────────────

TEST_CASE("LfoEngine LCG output always in [-1, +1] across 1000 calls", "[lfo][lcg][random]")
{
    LfoEngine lfo;
    auto p = baseParams();
    p.waveform = Waveform::Random;
    p.syncMode = false;
    p.rateHz   = 1.0f;

    for (int i = 0; i < 1000; ++i)
    {
        const float v = lfo.process(p);
        CHECK(v >= -1.0f);
        CHECK(v <= 1.0f);
    }
}
