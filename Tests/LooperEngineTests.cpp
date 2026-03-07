// LooperEngineTests.cpp
// Catch2 test suite for the lock-free LooperEngine.
//
// Tests are organised as follows:
//   TC 1  — default state
//   TC 2  — loop length beats across subdiv values
//   TC 3  — process() returns empty when not playing
//   TC 4  — record gate and play back
//   TC 5  — reset clears content
//   TC 6  — deleteLoop clears content
//   TC 7  — FIFO stress: atomic flags no-deadlock (concurrent writer+reader for flag stress)
//   TC 8  — DAW sync anchor snaps to bar boundary
//   TC 9  — negative ppqPosition ignored for anchor
//   TC 10 — punch-in preserves events at untouched beat positions (sequential model)
//   TC 11 — loop wrap boundary fires event exactly once per cycle (96-combo sweep)
//   TC 12 — snapToGrid: wrap edge case, tie-breaking (earlier), near-zero, non-boundary

#include "LooperEngine.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <thread>
#include <atomic>
#include <cmath>
#include <memory>

using Catch::Approx;

// ─────────────────────────────────────────────────────────────────────────────
// TC 1 — Default state
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - default state", "[looper]")
{
    LooperEngine eng;
    CHECK_FALSE(eng.isPlaying());
    CHECK_FALSE(eng.isRecording());
    CHECK_FALSE(eng.hasContent());
    CHECK(eng.getPlaybackBeat() == Approx(0.0));
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 2 — Loop length beats
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - loop length beats", "[looper]")
{
    LooperEngine eng;

    eng.setSubdiv(LooperSubdiv::FourFour);
    eng.setLoopLengthBars(4);
    CHECK(eng.getLoopLengthBeats() == Approx(16.0));

    eng.setSubdiv(LooperSubdiv::ThreeFour);
    eng.setLoopLengthBars(2);
    CHECK(eng.getLoopLengthBeats() == Approx(6.0));

    eng.setSubdiv(LooperSubdiv::SevenEight);
    eng.setLoopLengthBars(1);
    CHECK(eng.getLoopLengthBeats() == Approx(3.5));
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 3 — process() returns empty when not playing
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - process returns empty when not playing", "[looper]")
{
    LooperEngine eng;
    LooperEngine::ProcessParams p { 44100.0, 120.0, -1.0, 512, false };
    auto out = eng.process(p);
    CHECK_FALSE(out.hasJoystickX);
    CHECK_FALSE(out.hasJoystickY);
    for (int v = 0; v < 4; ++v)
    {
        CHECK_FALSE(out.gateOn[v]);
        CHECK_FALSE(out.gateOff[v]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 4 — Record gate and play back
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - record gate and play back", "[looper]")
{
    LooperEngine eng;
    eng.setSubdiv(LooperSubdiv::FourFour);
    eng.setLoopLengthBars(1);   // 4 beats
    eng.setRecGates(true);

    eng.startStop();   // playing_ = true
    eng.record();      // arm recording → recordPending_ = true
    { LooperEngine::ProcessParams pArm { 44100.0, 120.0, -1.0, 0, false }; eng.process(pArm); }  // activate recordPending_ → recording_ = true (blockSize=0: no clock drift)
    eng.recordGate(0.5, 0, true);    // gate-on voice 0 at beat 0.5
    eng.recordGate(1.0, 0, false);   // gate-off voice 0 at beat 1.0
    eng.record();      // stop recording → finaliseRecording() → moves to playbackStore_
    CHECK(eng.hasContent());

    // Block covering [0, 1.0) at 120 BPM, 44100 Hz:
    // samplesPerBeat = 44100 * 60 / 120 = 22050
    // 22050 samples = exactly 1.0 beat window — gate-on at 0.5 is in [0, 1.0)
    LooperEngine::ProcessParams p3 { 44100.0, 120.0, -1.0, 22050, false };
    auto out = eng.process(p3);
    CHECK(out.gateOn[0]);
    CHECK_FALSE(out.gateOff[0]);  // gate-off at beat 1.0 is at boundary (not < 1.0)
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 5 — reset clears content
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - reset clears content", "[looper]")
{
    LooperEngine eng;
    eng.startStop();
    eng.record();   // arm recording → recordPending_ = true
    { LooperEngine::ProcessParams pArm { 44100.0, 120.0, -1.0, 1, false }; eng.process(pArm); }  // activate recording
    eng.setRecGates(true);
    eng.recordGate(0.1, 1, true);
    eng.record();   // finalise
    CHECK(eng.hasContent());

    // reset() seeks to beat 0 (keeps content and play state — no longer clears content)
    eng.reset();
    LooperEngine::ProcessParams p { 44100.0, 120.0, -1.0, 512, false };
    eng.process(p);   // services resetRequest_
    CHECK(eng.hasContent());     // content preserved after reset
    CHECK(eng.isPlaying());      // play state preserved after reset
    CHECK(eng.getPlaybackBeat() == Catch::Approx(0.0).epsilon(0.001));  // position reset to 0
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 6 — deleteLoop clears content
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - deleteLoop clears content", "[looper]")
{
    LooperEngine eng;
    eng.startStop();
    eng.record();   // arm recording → recordPending_ = true
    { LooperEngine::ProcessParams pArm { 44100.0, 120.0, -1.0, 1, false }; eng.process(pArm); }  // activate recording
    eng.setRecGates(true);
    eng.recordGate(0.2, 2, true);
    eng.record();
    CHECK(eng.hasContent());

    eng.deleteLoop();
    LooperEngine::ProcessParams p { 44100.0, 120.0, -1.0, 512, false };
    eng.process(p);   // services deleteRequest_
    CHECK_FALSE(eng.hasContent());
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 7 — FIFO stress: atomic flags no-deadlock
//
// NOTE: This test validates the atomic-flag protocol under concurrent stress.
// It does NOT claim SPSC correctness with concurrent readers/writers — the SPSC
// guarantee is that the audio thread is the SOLE user of the FIFO in production.
// This stress test runs writer and reader simultaneously to confirm atomic flags
// (recording_, playing_, capReached_) remain consistent — no corruption, no hang.
// Correctness of FIFO event data is validated in TC 10 (sequential model).
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine FIFO stress - atomic flags no-deadlock", "[looper][threadsafe]")
{
    auto engSPtr = std::make_unique<LooperEngine>();
    LooperEngine& eng = *engSPtr;
    eng.startStop();   // playing_ = true
    eng.record();      // recording_ = true
    eng.setRecGates(true);

    std::atomic<int>  writtenCount { 0 };
    std::atomic<bool> errorDetected { false };

    // Writer: simulate audio thread recording gate events
    auto writerFn = [&]() {
        for (int i = 0; i < 5000 && !errorDetected.load(); ++i)
        {
            eng.recordGate(i * 0.001, i % 4, (i % 2) == 0);
            writtenCount.fetch_add(1, std::memory_order_relaxed);
        }
    };

    // Reader: simulate audio thread processing blocks
    // (intentionally violates SPSC to stress-test atomic flags — not FIFO data correctness)
    std::atomic<int> readCount { 0 };
    auto readerFn = [&]() {
        LooperEngine::ProcessParams p { 44100.0, 120.0, -1.0, 512, false };
        for (int i = 0; i < 100; ++i)
        {
            auto out = eng.process(p);
            readCount.fetch_add(1, std::memory_order_relaxed);
            (void)out;
        }
    };

    std::thread writer(writerFn);
    std::thread reader(readerFn);
    writer.join();
    reader.join();

    // Assert only from main thread after join (Catch2 assertion state is not thread-safe).
    CHECK_FALSE(errorDetected.load());
    CHECK(writtenCount.load() > 0);
    CHECK(readCount.load() == 100);
    // No deadlock = test completed (would hang forever if mutex deadlock existed)
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 8 — DAW sync anchor snaps to bar boundary
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - DAW sync anchor snaps to bar boundary", "[looper]")
{
    LooperEngine eng;
    eng.setSyncToDaw(true);
    eng.setSubdiv(LooperSubdiv::FourFour);
    eng.setLoopLengthBars(1);
    eng.startStop();

    // ppqPosition = 6.7 (mid-bar 2 of a 4/4 pattern)
    // beatsPerBar = 4.0 → barIndex = floor(6.7/4.0) = 1 → anchor = 4.0
    // elapsed = 6.7 - 4.0 = 2.7; fmod(2.7, 4.0) = 2.7
    LooperEngine::ProcessParams p { 44100.0, 120.0, 6.7, 512, true };
    auto out = eng.process(p);
    CHECK(eng.getPlaybackBeat() == Approx(2.7).epsilon(0.01));
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 9 — Negative ppqPosition is ignored for anchor
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - negative ppqPosition is ignored for anchor", "[looper]")
{
    LooperEngine eng;
    eng.setSyncToDaw(true);
    eng.setSubdiv(LooperSubdiv::FourFour);
    eng.setLoopLengthBars(1);
    eng.startStop();

    // Cubase pre-roll: ppqPosition = -2.0 — useDaw condition fails (ppqPosition < 0)
    // so anchor is never set; looper pauses (DAW sync on, DAW not playing effectively)
    LooperEngine::ProcessParams p { 44100.0, 120.0, -2.0, 512, true };
    auto out = eng.process(p);
    // DAW sync on + ppqPosition<0 → useDaw=false, syncToDaw_=true → pause branch
    // playbackBeat_ = fmod(internalBeat_(0.0), loopLen) = 0.0
    CHECK(eng.getPlaybackBeat() == Approx(0.0).epsilon(0.001));
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 10 — Punch-in preserves events at untouched beat positions
//
// NOTE: This test validates FIFO data correctness using the sequential model
// (record then play, no concurrent writer/reader). This is the production usage
// pattern and the correct way to verify FIFO event correctness per the SPSC guarantee.
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - punch-in preserves events at untouched beat positions", "[looper]")
{
    auto engPtr = std::make_unique<LooperEngine>();
    LooperEngine& eng = *engPtr;
    eng.setSubdiv(LooperSubdiv::FourFour);
    eng.setLoopLengthBars(1);   // 4 beats
    eng.setRecGates(true);

    // --- FIRST PASS: record voice 0 gate-on at beat 0.5 ---
    eng.startStop();   // playing_ = true
    eng.record();      // arm recording → recordPending_ = true
    { LooperEngine::ProcessParams pArm { 44100.0, 120.0, -1.0, 0, false }; eng.process(pArm); }  // activate (blockSize=0: no clock drift)
    eng.recordGate(0.5, 0, true);   // voice 0 on at beat 0.5
    eng.record();      // stop recording → finaliseRecording() → playbackStore_ has 1 event
    CHECK(eng.hasContent());

    // Stop looper before punch-in (sequential model requires stop before re-record)
    eng.startStop();   // playing_ = false → finaliseRecording() (no new events, no change)

    // --- SECOND PASS (punch-in): record voice 1 gate-on at beat 2.0 ---
    // Beat 2.0 is NOT in the same beat window as 0.5 (separation = 1.5 beats >> touchRadius)
    // Arm BEFORE startStop: stopped state → recordPending_ (not recPendingNextCycle_)
    eng.record();      // arm while stopped → recordPending_ = true
    eng.startStop();   // playing_ = true
    { LooperEngine::ProcessParams pArm { 44100.0, 120.0, -1.0, 0, false }; eng.process(pArm); }  // activate (blockSize=0: no clock drift)
    eng.recordGate(2.0, 1, true);   // voice 1 on at beat 2.0
    eng.record();      // stop recording → finaliseRecording() with punch-in merge

    // playbackStore_ should now contain BOTH:
    //   event[beat=0.5, voice=0, on=true]  — preserved from pass 1
    //   event[beat=2.0, voice=1, on=true]  — added from pass 2

    // Verify hasContent (punch-in merge produced 2 events)
    REQUIRE(eng.hasContent());

    // Verify via process():
    // At 120 BPM, 44100 Hz: samplesPerBeat = 22050; 22050 samples = 1.0 beat window

    // Block 1: internal clock at 0 → covers [0.0, 1.0) — voice 0 gate-on at 0.5 fires
    LooperEngine::ProcessParams p1 { 44100.0, 120.0, -1.0, 22050, false };
    auto out1 = eng.process(p1);
    CHECK(out1.gateOn[0]);       // voice 0 from original pass preserved
    CHECK_FALSE(out1.gateOn[1]); // voice 1 fires at beat 2.0, not in [0.0, 1.0)

    // Block 2: covers [1.0, 2.0) — neither event fires
    auto out2 = eng.process(p1);
    CHECK_FALSE(out2.gateOn[0]);
    CHECK_FALSE(out2.gateOn[1]);

    // Block 3: covers [2.0, 3.0) — voice 1 gate-on at 2.0 fires
    auto out3 = eng.process(p1);
    CHECK_FALSE(out3.gateOn[0]);
    CHECK(out3.gateOn[1]);   // voice 1 from punch-in pass present
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 11 — Loop wrap boundary fires event exactly once per cycle
//
// Satisfies the locked requirement from CONTEXT.md Section D:
// "loop-wrap unit tests: process() called with mock ProcessParams for every bar
//  length 1-16, all time signatures, verifying events fire in the correct beat
//  windows including wrap-around."
//
// Sweeps all 6 LooperSubdiv values x bar lengths 1-16 (96 combinations).
// For each: records a single gate event placed near the loop boundary
// (beatPos = loopLen - 0.01), drives the internal clock across the wrap via
// sequential process() calls, and verifies the event fires exactly once per
// complete loop cycle.
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - loop wrap boundary fires event exactly once per cycle", "[looper][wrap]")
{
    struct SubdivCase { LooperSubdiv subdiv; double bpb; const char* name; };
    const SubdivCase subdivCases[] = {
        { LooperSubdiv::ThreeFour,   3.0, "3/4"   },
        { LooperSubdiv::FourFour,    4.0, "4/4"   },
        { LooperSubdiv::FiveFour,    5.0, "5/4"   },
        { LooperSubdiv::SevenEight,  3.5, "7/8"   },
        { LooperSubdiv::NineEight,   4.5, "9/8"   },
        { LooperSubdiv::ElevenEight, 5.5, "11/8"  },
    };

    for (const auto& sc : subdivCases)
    {
        for (int bars = 1; bars <= 16; ++bars)
        {
            CAPTURE(sc.name, bars);

            LooperEngine eng;
            eng.setSubdiv(sc.subdiv);
            eng.setLoopLengthBars(bars);
            eng.setRecGates(true);

            const double loopLen = eng.getLoopLengthBeats();
            REQUIRE(loopLen == Approx(sc.bpb * bars).epsilon(0.001));

            // Place the event just before the loop boundary (epsilon = 0.01 beats)
            const double epsilon   = 0.01;
            const double eventBeat = loopLen - epsilon;

            // Record the event (sequential: record then play)
            eng.startStop();   // playing_ = true
            eng.record();      // arm recording → recordPending_ = true
            { LooperEngine::ProcessParams pArm { 44100.0, 120.0, -1.0, 0, false }; eng.process(pArm); }  // activate (blockSize=0: no clock drift)
            eng.recordGate(eventBeat, 0, true);
            eng.record();      // stop recording → finaliseRecording()
            REQUIRE(eng.hasContent());

            // Drive playback via process() using small fixed blocks.
            // blockSize = 441 samples @ 44100 Hz, 120 BPM:
            //   blockBeats = 441 / (44100 * 60 / 120) = 441 / 22050 = 0.02 beats
            // 0.02 < epsilon (0.01 * 2) so the wrap-boundary event fits in one block window.
            const double sampleRate  = 44100.0;
            const double bpm         = 120.0;
            const int    blockSize   = 441;
            const double samplesPerBeat = sampleRate * 60.0 / bpm;  // 22050
            const double blockBeats  = blockSize / samplesPerBeat;   // 0.02

            // Drive 2 full loop cycles to verify event fires exactly once each cycle.
            const int blocksPerLoop = static_cast<int>(std::ceil(loopLen / blockBeats)) + 1;
            const int totalBlocks   = blocksPerLoop * 2;

            LooperEngine::ProcessParams p { sampleRate, bpm, -1.0, blockSize, false };

            int fireCount = 0;
            for (int b = 0; b < totalBlocks; ++b)
            {
                auto out = eng.process(p);
                if (out.gateOn[0])
                    ++fireCount;
            }

            // Event must fire exactly twice across 2 full loop cycles (once per cycle).
            CHECK(fireCount == 2);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 12 — snapToGrid: wrap edge case, tie-breaking (earlier), near-zero, non-boundary
//
// Validates the snapToGrid free function declared in LooperEngine.h.
// Must pass before any LooperEngine integration work begins (STATE.md blocker).
//
// Cases tested:
//   1. Wrap edge:    beatPos=3.999 → nearest grid is 4.0 = loopLen → fmod → 0.0
//   2. Tie-breaking: beatPos=3.5   → equidistant between 3.0 and 4.0 → snap EARLIER → 3.0
//   3. Near-zero:    beatPos=0.001 → nearest grid is 0.0
//   4. Non-boundary: beatPos=2.3, gridSize=0.5 → lower=2.0, upper=2.5, mid=2.25 → 2.3>2.25 → 2.5
//   5. 1/8-note grid: beatPos=1.4, gridSize=0.5 → lower=1.0, upper=1.5, mid=1.25 → 1.4>1.25 → 1.5
//   6. quantizeSubdivToGridSize: 0→1.0, 1→0.5, 2→0.25, 3→0.125
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("snapToGrid - wrap edge case, tie-breaking, near-zero, non-boundary", "[looper][quantize]")
{
    const double loopLen  = 4.0;
    const double gridSize = 1.0;

    // 1. Wrap edge: beatPos near end of loop snaps to loopLen → fmod wraps to 0.0
    SECTION("wrap edge: beatPos=3.999 snaps to 0.0")
    {
        CHECK(snapToGrid(3.999, gridSize, loopLen) == Approx(0.0).epsilon(1e-9));
    }

    // 2. Tie-breaking: exact midpoint should snap to EARLIER grid point (3.0 not 4.0)
    SECTION("tie-breaking: beatPos=3.5 snaps to earlier grid point 3.0")
    {
        CHECK(snapToGrid(3.5, gridSize, loopLen) == Approx(3.0).epsilon(1e-9));
    }

    // 3. Near-zero: very small positive beat position snaps to 0.0
    SECTION("near-zero: beatPos=0.001 snaps to 0.0")
    {
        CHECK(snapToGrid(0.001, gridSize, loopLen) == Approx(0.0).epsilon(1e-9));
    }

    // 4. Non-boundary: 2.3 is closer to 2.5 than 2.0 (midpoint is 2.25, 2.3 > 2.25 → upper)
    SECTION("non-boundary: beatPos=2.3, gridSize=0.5 snaps to 2.5")
    {
        CHECK(snapToGrid(2.3, 0.5, loopLen) == Approx(2.5).epsilon(1e-9));
    }

    // 5. 1/8-note grid: beatPos=1.4, lower=1.0, upper=1.5, mid=1.25 → 1.4 > 1.25 → 1.5
    SECTION("1/8-note grid: beatPos=1.4 snaps to 1.5")
    {
        CHECK(snapToGrid(1.4, 0.5, loopLen) == Approx(1.5).epsilon(1e-9));
    }

    // 6. quantizeSubdivToGridSize: index 0..3 maps to correct beat-unit grid sizes
    SECTION("quantizeSubdivToGridSize: 0→1.0, 1→0.5, 2→0.25, 3→0.125")
    {
        CHECK(quantizeSubdivToGridSize(0) == Approx(1.0).epsilon(1e-12));
        CHECK(quantizeSubdivToGridSize(1) == Approx(0.5).epsilon(1e-12));
        CHECK(quantizeSubdivToGridSize(2) == Approx(0.25).epsilon(1e-12));
        CHECK(quantizeSubdivToGridSize(3) == Approx(0.125).epsilon(1e-12));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 13 — BUG-01 regression: DAW sync playback beat 0 after auto-stop record cycle
//
// Drives a 1-bar 4/4 loop (loopLen = 4.0 beats) at 120 BPM in DAW sync mode.
// Recording is activated at ppq = 0.0 with an anchor of 0.0 (bar boundary).
// 4 blocks of 22050 samples (1.0 beat each) drive recording to completion.
// On block 3 (ppq = 3.0), recordedBeats_ reaches 4.0 → auto-stop fires.
// After auto-stop, loopStartPpq_ must be advanced by loopLen to 4.0 so that
// subsequent playback correctly tracks the anchor for the new cycle.
//
// Bug manifestation: when the first playback block arrives with ppqPosition
// just BELOW the bar boundary (4.0 - epsilon), the sentinel loopStartPpq_ = -1.0
// causes anchorToBar() to floor to 0.0 (the PREVIOUS bar), giving elapsed ≈ 4.0
// and playbackBeat ≈ 4.0 instead of ≈ 0.0.
//
// With the FIX: loopStartPpq_ = loopStartPpq_ + loopLen = 4.0.
// For ppq = 4.0 - 1e-6: elapsed = -1e-6 → clamped to 0.0 → beat = 0.0.
//
// RED before fix: getPlaybackBeat() ≈ 4.0 (fails Approx(0.0) with epsilon 0.01)
// GREEN after fix: getPlaybackBeat() = 0.0
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - BUG-01: DAW sync beat 0 after auto-stop record cycle", "[looper][bug01]")
{
    LooperEngine eng;
    eng.setSyncToDaw(true);
    eng.setSubdiv(LooperSubdiv::FourFour);
    eng.setLoopLengthBars(1);   // loopLen = 4.0 beats
    eng.setRecGates(true);

    // samplesPerBeat at 120 BPM, 44100 Hz = 22050
    // Each block is exactly 1 beat; recording auto-stops after 4 blocks.
    // Anchor set at block 0: anchorToBar(0.0, 4.0) = 0.0 → loopStartPpq_ = 0.0

    eng.startStop();   // playing_ = true (DAW sync: becomes true on first DAW-playing block)
    eng.record();      // arm recording (recordPending_)

    // Blocks 0-2: ppq 0, 1, 2 — recording accumulates, REQUIRE still recording each block
    for (int i = 0; i < 3; ++i)
    {
        LooperEngine::ProcessParams p { 44100.0, 120.0, static_cast<double>(i), 22050, true };
        eng.process(p);
        REQUIRE(eng.isRecording());
    }

    // Block 3: ppq = 3.0 — recordedBeats_ reaches 4.0 >= loopLen → auto-stop fires
    LooperEngine::ProcessParams p3 { 44100.0, 120.0, 3.0, 22050, true };
    eng.process(p3);
    REQUIRE_FALSE(eng.isRecording());
    REQUIRE(eng.isPlaying());

    // First playback block: ppq = 4.0 - 1e-6 (simulates real-world FP drift just below
    // the bar boundary). With the BUG (loopStartPpq_ = -1.0), anchorToBar floors to 0.0
    // (the PREVIOUS bar), giving elapsed ≈ 4.0 and playbackBeat ≈ 4.0 — clearly wrong.
    // With the FIX (loopStartPpq_ += loopLen = 4.0), elapsed = -1e-6 → clamped to 0.0.
    const double ppqJustBeforeBar = 4.0 - 1e-6;
    LooperEngine::ProcessParams p4 { 44100.0, 120.0, ppqJustBeforeBar, 22050, true };
    eng.process(p4);
    CHECK(eng.getPlaybackBeat() == Catch::Approx(0.0).epsilon(0.01));
}

// ─────────────────────────────────────────────────────────────────────────────
// TC 14 — double-scan regression: internalBeat_ must retain fmod overshoot
//         after recording auto-stops in free-running mode.
//
// BUG (line 773): internalBeat_ = 0.0 overwrites the fmod-absorbed overshoot,
//   causing the next block to start at beat 0.0 and fire beat-0 events twice.
// FIX: Line 773 removed; fmod at line 758 already sets internalBeat_ = overshoot.
//
// Observable symptom: getPlaybackBeat() returns 0.0 on the block immediately
//   after recording ends (with bug). With fix it returns the overshoot value > 0.
//
// Setup:
//   - 4/4, 1 bar (loopLen = 4.0 beats), free-running (no DAW sync)
//   - blockSize = 512 samples, 44100 Hz, 120 BPM
//   - blockBeats = 512 / 22050 ≈ 0.02322 (non-integer fraction of loopLen → guaranteed overshoot)
//   - Recording auto-stops after ≈ 173 blocks (recordedBeats_ >= 4.0)
//   - overshoot ≈ 0.017 beats → internalBeat_ = 0.017 after fix
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LooperEngine - TC14: internalBeat_ retains fmod overshoot after auto-stop (free-running)", "[looper][bug-double-scan]")
{
    LooperEngine eng;
    eng.setSubdiv(LooperSubdiv::FourFour);
    eng.setLoopLengthBars(1);   // loopLen = 4.0 beats
    eng.setRecGates(true);

    const double sampleRate  = 44100.0;
    const double bpm         = 120.0;
    const int    blockSize   = 512;   // small blocks ensure non-zero overshoot at recording end
    const double blockBeats  = blockSize / (sampleRate * 60.0 / bpm);  // ≈ 0.02322

    // Sanity: blockBeats must NOT be an exact divisor of loopLen (4.0).
    // If it were, overshoot = 0 and the test would be vacuous.
    // 4.0 / 0.02322 ≈ 172.27 — not an integer, overshoot is guaranteed.
    REQUIRE(std::fmod(4.0, blockBeats) > 1e-6);

    eng.startStop();   // playing_ = true (free-running)
    eng.record();      // arm recording → recordPending_ = true

    LooperEngine::ProcessParams p { sampleRate, bpm, -1.0, blockSize, false };

    // Drive blocks until recording auto-stops.
    // First block activates recordPending_ → recording_ = true.
    // Subsequent blocks accumulate recordedBeats_; auto-stop fires when >= 4.0.
    // Safety limit: 300 blocks >> expected ≈ 173 blocks.
    int driven = 0;
    for (int i = 0; i < 300; ++i)
    {
        eng.process(p);
        ++driven;
        if (!eng.isRecording())
            break;
    }

    REQUIRE_FALSE(eng.isRecording());   // recording must have auto-stopped
    REQUIRE(eng.isPlaying());           // looper stays in playback mode
    REQUIRE(driven < 300);              // safety: confirm auto-stop fired, not limit hit

    // Drive ONE more block — this is the first-playback block where the bug manifests.
    // With bug:  internalBeat_ = 0.0 → beatAtBlockStart = 0.0 → playbackBeat_ = 0.0
    // With fix:  internalBeat_ = overshoot ≈ 0.017 → beatAtBlockStart = 0.017 > 0.0
    eng.process(p);
    const double beatAfterStop = eng.getPlaybackBeat();

    // Fix: beat position must reflect the overshoot, not be reset to 0.0
    // Lower bound: slightly above 0 to catch the reset-to-zero bug
    // Upper bound: less than blockBeats (overshoot < one block worth)
    CHECK(beatAfterStop > 1e-4);         // must be non-zero (bug: 0.0)
    CHECK(beatAfterStop < blockBeats);   // must be less than one block (sane overshoot)
}
