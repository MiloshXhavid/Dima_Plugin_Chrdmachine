#include "LooperEngine.h"
#include <cmath>
#include <algorithm>

// No #include <mutex> — this file contains zero blocking calls.

// ─── Configuration helpers ────────────────────────────────────────────────────

double LooperEngine::beatsPerBar() const
{
    switch ((LooperSubdiv)subdiv_.load(std::memory_order_relaxed))
    {
        case LooperSubdiv::ThreeFour:   return 3.0;
        case LooperSubdiv::FourFour:    return 4.0;
        case LooperSubdiv::FiveFour:    return 5.0;
        case LooperSubdiv::SevenEight:  return 3.5;   // 7 eighth-notes = 3.5 quarter-notes
        case LooperSubdiv::NineEight:   return 4.5;
        case LooperSubdiv::ElevenEight: return 5.5;
    }
    return 4.0;
}

double LooperEngine::getLoopLengthBeats() const
{
    return beatsPerBar() * static_cast<double>(loopBars_.load(std::memory_order_relaxed));
}

bool LooperEngine::hasContent() const
{
    // No mutex — getNumReady() is atomic internally in AbstractFifo.
    return playbackCount_.load(std::memory_order_relaxed) > 0
        || fifo_.getNumReady() > 0;
}

void LooperEngine::setLoopLengthBars(int bars)
{
    loopBars_.store(juce::jlimit(1, 16, bars));
}

// ─── Transport ────────────────────────────────────────────────────────────────

void LooperEngine::startStop()
{
    const bool wasPlaying = playing_.load();
    if (!wasPlaying)
    {
        playing_.store(true);
    }
    else
    {
        // Stop: freeze current FIFO content into playbackStore_.
        // INVARIANT: recording_ must be false before finaliseRecording().
        playing_.store(false);
        recording_.store(false);
        finaliseRecording();
    }
}

void LooperEngine::record()
{
    if (recording_.load())
    {
        // Already recording → disarm, merge FIFO into playbackStore_ (punch-in logic).
        // INVARIANT: recording_ must be false before finaliseRecording().
        recording_.store(false);
        finaliseRecording();
        return;
    }

    if (recordPending_.load())
    {
        // Pending record was already armed — second REC press cancels it.
        recordPending_.store(false);
        return;
    }

    // Cancel wait-for-trigger arm if already waiting.
    if (recWaitArmed_.load())
    {
        recWaitArmed_.store(false);
        return;
    }

    // Arm: start playing if not already (REC implies PLAY).
    if (!playing_.load())
        playing_.store(true);

    // If wait-for-trigger mode is ON, arm the wait state instead of arming
    // recordPending_. Recording will start when activateRecordingNow() is called
    // from the audio thread on the first note-on.
    if (recWaitForTrigger_.load())
    {
        recWaitArmed_.store(true);
        return;
    }

    // Defer the actual recording_.store(true) to process() so it begins on the
    // next valid clock: immediately in free-running mode, or when DAW starts
    // rolling in SYNC mode.
    recordPending_.store(true);
}

void LooperEngine::reset()
{
    // Audio thread services this at the top of process().
    resetRequest_.store(true, std::memory_order_release);
}

void LooperEngine::deleteLoop()
{
    // Audio thread services this at the top of process().
    deleteRequest_.store(true, std::memory_order_release);
}

// ─── activateRecordingNow (audio-thread-only) ─────────────────────────────────

void LooperEngine::activateRecordingNow()
{
    ASSERT_AUDIO_THREAD();
    if (!recWaitArmed_.load(std::memory_order_relaxed)) return;
    recWaitArmed_.store(false, std::memory_order_relaxed);
    capReached_.store(false, std::memory_order_relaxed);
    fifo_.reset();
    recordedBeats_ = 0.0;
    recording_.store(true, std::memory_order_relaxed);
}

// ─── DAW sync anchor ─────────────────────────────────────────────────────────

void LooperEngine::anchorToBar(double ppqPosition, double bpb)
{
    const double barIndex = std::floor(ppqPosition / bpb);
    loopStartPpq_ = barIndex * bpb;
}

// ─── Punch-in merge ───────────────────────────────────────────────────────────
//
// THREADING INVARIANT:
//   This method MUST only be called when BOTH conditions are true:
//     (a) playing_ = false  — process() early-returns before touching the FIFO or
//         playbackStore_, so there is no concurrent reader.
//     (b) recording_ = false — recordGate() and recordJoystick() both early-return
//         before writing to the FIFO, so there is no concurrent writer.
//   Both callers (startStop() and record()) set these flags before calling this method.
//   DO NOT call finaliseRecording() with playing_=true or recording_=true.

void LooperEngine::finaliseRecording()
{
    const int newCount = fifo_.getNumReady();

    // Case 0: no new events recorded — nothing to merge; leave playbackStore_ unchanged.
    if (newCount == 0)
        return;

    // Case 1: no existing playback content — simple drain of FIFO into playbackStore_.
    if (playbackCount_.load(std::memory_order_relaxed) == 0)
    {
        int count = 0;
        // Scope block ensures ScopedRead destructs (calls finishedRead) BEFORE reset().
        // If reset() were called while ScopedRead is alive, its destructor would advance
        // validStart past 0, leaving validStart=newCount with validEnd=0, making
        // getNumReady() return bufferSize-newCount instead of 0.
        {
            auto scope = fifo_.read(newCount);
            scope.forEach([&](int idx)
            {
                if (count < LOOPER_FIFO_CAPACITY)
                    playbackStore_[count++] = eventBuf_[idx];
            });
        }  // ScopedRead destructs here: finishedRead(newCount) → validStart=newCount
        fifo_.reset();  // now safe: validStart=validEnd=newCount → reset to 0,0
        playbackCount_.store(count, std::memory_order_release);
        // fall through to open-gate choke below
    }
    else
    {

    // Case 2: punch-in merge — existing content AND new events.
    // Step A: drain new events from FIFO into scratchNew_ (class member — avoids ~49KB stack alloc).
    int newEventCount = 0;
    {
        auto scope = fifo_.read(newCount);
        scope.forEach([&](int idx)
        {
            if (newEventCount < LOOPER_FIFO_CAPACITY)
                scratchNew_[newEventCount++] = eventBuf_[idx];
        });
    }
    fifo_.reset();

    // Step B: build merge result.
    // "Touched" beat window: within ±touchRadius of any newly recorded event.
    // touchRadius ≈ 1/64th of the loop length — sufficient for block-granular recording.
    const double loopLen     = getLoopLengthBeats();
    const double touchRadius = (loopLen > 0.0) ? (loopLen / 64.0) : 0.05;

    // Step C: merged result stored in scratchMerged_ (class member — avoids ~49KB stack alloc).
    int mergedCount = 0;

    // First pass: keep old events that fall OUTSIDE all touched beat windows.
    const int oldCount = playbackCount_.load(std::memory_order_relaxed);
    for (int i = 0; i < oldCount && mergedCount < LOOPER_FIFO_CAPACITY; ++i)
    {
        const double oldPos = playbackStore_[i].beatPosition;
        bool inTouchedWindow = false;
        for (int j = 0; j < newEventCount; ++j)
        {
            const double dist = std::abs(oldPos - scratchNew_[j].beatPosition);
            // Handle wrap-around at loop boundary
            const double distWrapped = std::min(dist, loopLen - dist);
            if (distWrapped <= touchRadius)
            {
                inTouchedWindow = true;
                break;
            }
        }
        if (!inTouchedWindow)
            scratchMerged_[mergedCount++] = playbackStore_[i];
    }

    // Second pass: append all new events.
    for (int j = 0; j < newEventCount && mergedCount < LOOPER_FIFO_CAPACITY; ++j)
        scratchMerged_[mergedCount++] = scratchNew_[j];

    // Copy merged result back into playbackStore_.
    for (int i = 0; i < mergedCount; ++i)
        playbackStore_[i] = scratchMerged_[i];
    playbackCount_.store(mergedCount, std::memory_order_release);

    } // end Case 2

    // ── Choke open gates at loop boundary ────────────────────────────────────
    // If the last gate event for a voice is "on", the note would hold forever on
    // playback (gate-on was recorded while touchplate was still held at loop end).
    // Inject a gate-off near the very end of the loop so the note is cut cleanly
    // before the next cycle restarts.
    {
        int n = playbackCount_.load(std::memory_order_relaxed);
        const double chopBeat = getLoopLengthBeats() * (1.0 - 1e-9);

        // Per-voice: find the gate event with the highest beatPosition (final state).
        double lastBeat [4] = { -1.0, -1.0, -1.0, -1.0 };
        float  lastValue[4] = {  0.0f, 0.0f, 0.0f, 0.0f };

        for (int i = 0; i < n; ++i)
        {
            const auto& ev = playbackStore_[i];
            if (ev.type != LooperEvent::Type::Gate) continue;
            if (ev.voice < 0 || ev.voice >= 4)      continue;
            if (ev.beatPosition > lastBeat[ev.voice])
            {
                lastBeat [ev.voice] = ev.beatPosition;
                lastValue[ev.voice] = ev.value;
            }
        }

        for (int v = 0; v < 4 && n < LOOPER_FIFO_CAPACITY; ++v)
        {
            if (lastBeat[v] >= 0.0 && lastValue[v] > 0.5f)
                playbackStore_[n++] = { chopBeat, LooperEvent::Type::Gate, v, 0.0f };
        }

        playbackCount_.store(n, std::memory_order_release);
    }
}

// ─── Recording helpers (audio-thread-only) ────────────────────────────────────

void LooperEngine::recordFilter(double beatPos, float x, float y)
{
    ASSERT_AUDIO_THREAD();
    if (!recording_.load(std::memory_order_relaxed))   return;
    if (!recFilter_.load(std::memory_order_relaxed))   return;

    const double loopLen = getLoopLengthBeats();
    if (loopLen <= 0.0) return;

    const float thresh = joystickThresh_.load(std::memory_order_relaxed);
    const bool  xMoved = std::abs(x - lastRecordedFilterX_) > thresh;
    const bool  yMoved = std::abs(y - lastRecordedFilterY_) > thresh;
    if (!xMoved && !yMoved) return;

    const double pos = std::fmod(beatPos, loopLen);

    if (xMoved)
    {
        if (fifo_.getFreeSpace() < 1) { capReached_.store(true, std::memory_order_relaxed); return; }
        LooperEvent ex { pos, LooperEvent::Type::FilterX, -1, x };
        auto scope = fifo_.write(1);
        scope.forEach([&](int idx) { eventBuf_[idx] = ex; });
        lastRecordedFilterX_ = x;
    }
    if (yMoved)
    {
        if (fifo_.getFreeSpace() < 1) { capReached_.store(true, std::memory_order_relaxed); return; }
        LooperEvent ey { pos, LooperEvent::Type::FilterY, -1, y };
        auto scope = fifo_.write(1);
        scope.forEach([&](int idx) { eventBuf_[idx] = ey; });
        lastRecordedFilterY_ = y;
    }
}

void LooperEngine::recordGate(double beatPos, int voice, bool on)
{
    ASSERT_AUDIO_THREAD();
    if (!recording_.load(std::memory_order_relaxed)) return;
    if (!recGates_.load(std::memory_order_relaxed))  return;

    const double loopLen = getLoopLengthBeats();
    if (loopLen <= 0.0) return;

    // Cap check — auto-stop recording when FIFO is full.
    if (fifo_.getFreeSpace() < 1)
    {
        capReached_.store(true, std::memory_order_relaxed);
        recording_.store(false, std::memory_order_relaxed);
        return;
    }

    const double pos = std::fmod(beatPos, loopLen);
    LooperEvent e { pos, LooperEvent::Type::Gate, voice, on ? 1.0f : 0.0f };

    auto scope = fifo_.write(1);
    scope.forEach([&](int idx) { eventBuf_[idx] = e; });
}

void LooperEngine::recordJoystick(double beatPos, float x, float y)
{
    ASSERT_AUDIO_THREAD();
    if (!recording_.load(std::memory_order_relaxed)) return;
    if (!recJoy_.load(std::memory_order_relaxed))    return;

    const double loopLen = getLoopLengthBeats();
    if (loopLen <= 0.0) return;

    // Sparse recording: only write when movement exceeds threshold.
    const float thresh = joystickThresh_.load(std::memory_order_relaxed);
    const bool  xMoved = std::abs(x - lastRecordedX_) > thresh;
    const bool  yMoved = std::abs(y - lastRecordedY_) > thresh;
    if (!xMoved && !yMoved) return;

    const double pos = std::fmod(beatPos, loopLen);

    if (xMoved)
    {
        if (fifo_.getFreeSpace() < 1)
        {
            capReached_.store(true, std::memory_order_relaxed);
            return;
        }
        LooperEvent ex { pos, LooperEvent::Type::JoystickX, -1, x };
        auto scope = fifo_.write(1);
        scope.forEach([&](int idx) { eventBuf_[idx] = ex; });
        lastRecordedX_ = x;
    }

    if (yMoved)
    {
        if (fifo_.getFreeSpace() < 1)
        {
            capReached_.store(true, std::memory_order_relaxed);
            return;
        }
        LooperEvent ey { pos, LooperEvent::Type::JoystickY, -1, y };
        auto scope = fifo_.write(1);
        scope.forEach([&](int idx) { eventBuf_[idx] = ey; });
        lastRecordedY_ = y;
    }
}

// ─── Audio-thread process ─────────────────────────────────────────────────────

LooperEngine::BlockOutput LooperEngine::process(const ProcessParams& p)
{
    ASSERT_AUDIO_THREAD();
    BlockOutput out {};

    // Service destructive requests first.
    // Safe: audio thread is the sole FIFO user; no concurrent reader/writer at this point.
    if (resetRequest_.exchange(false, std::memory_order_acq_rel))
    {
        fifo_.reset();
        playbackStore_ = {};
        playbackCount_.store(0, std::memory_order_relaxed);
        internalBeat_  = 0.0;
        loopStartPpq_  = -1.0;
        playing_.store(false, std::memory_order_relaxed);
        recording_.store(false, std::memory_order_relaxed);
        playbackBeat_.store(0.0f, std::memory_order_relaxed);
        return out;
    }

    if (deleteRequest_.exchange(false, std::memory_order_acq_rel))
    {
        fifo_.reset();
        playbackStore_ = {};
        playbackCount_.store(0, std::memory_order_relaxed);
        return out;
    }

    // ── DAW transport sync: auto-stop / auto-start with DAW when SYNC is enabled ─
    {
        const bool syncOn    = syncToDaw_.load(std::memory_order_relaxed);
        const bool dawActive = syncOn && p.isDawPlaying && p.ppqPosition >= 0.0;

        if (syncOn)
        {
            if (!dawActive && prevDawPlaying_)
            {
                // DAW just stopped → stop looper, reset anchor, signal all-notes-off
                playing_.store(false, std::memory_order_relaxed);
                loopStartPpq_   = -1.0;
                prevDawPlaying_ = false;
                out.dawStopped  = true;
                return out;
            }
            if (dawActive && !prevDawPlaying_)
            {
                // DAW just started → restart if has content or REC was armed while DAW was stopped
                if (playbackCount_.load(std::memory_order_relaxed) > 0
                    || recordPending_.load(std::memory_order_relaxed))
                    playing_.store(true, std::memory_order_relaxed);
            }
            prevDawPlaying_ = dawActive;
        }
        else
        {
            prevDawPlaying_ = false;  // reset so transition is detected if SYNC re-enabled
        }
    }

    if (!playing_.load(std::memory_order_relaxed)) return out;

    const double loopLen = getLoopLengthBeats();
    if (loopLen <= 0.0) return out;

    const double bpm          = (p.bpm > 0.0) ? p.bpm : 120.0;
    const double samplesPerBeat = (p.sampleRate * 60.0) / bpm;

    // ── Determine beat position at block start ────────────────────────────────
    double beatAtBlockStart;
    const bool useDaw = syncToDaw_.load(std::memory_order_relaxed)
                     && p.isDawPlaying
                     && p.ppqPosition >= 0.0;

    if (useDaw)
    {
        // DAW sync: anchor loopStartPpq_ to the nearest bar boundary on first call.
        if (loopStartPpq_ < 0.0)
            anchorToBar(p.ppqPosition, beatsPerBar());

        const double elapsed = p.ppqPosition - loopStartPpq_;
        beatAtBlockStart = std::fmod(elapsed < 0.0 ? 0.0 : elapsed, loopLen);
    }
    else
    {
        if (syncToDaw_.load(std::memory_order_relaxed))
        {
            // DAW sync is ON but DAW is not playing — pause internal advance.
            playbackBeat_.store((float)std::fmod(internalBeat_, loopLen), std::memory_order_relaxed);
            return out;
        }
        // Internal clock: reset DAW anchor when switching back.
        loopStartPpq_    = -1.0;
        beatAtBlockStart = std::fmod(internalBeat_, loopLen);
    }

    // Activate pending recording — we only reach here when free-running OR DAW is actively
    // playing (the "SYNC on, DAW stopped" path returns early above), so it's safe to start.
    if (recordPending_.load(std::memory_order_relaxed))
    {
        recordPending_.store(false, std::memory_order_relaxed);
        capReached_.store(false, std::memory_order_relaxed);
        fifo_.reset();   // clean FIFO before new recording pass begins
        recording_.store(true, std::memory_order_relaxed);
        recordedBeats_ = 0.0;   // reset counter for fresh recording pass
    }

    const double blockBeats = static_cast<double>(p.blockSize) / samplesPerBeat;
    const double rawEnd     = beatAtBlockStart + blockBeats;
    const double beatAtEnd  = std::fmod(rawEnd, loopLen);
    const bool   wraps      = rawEnd >= loopLen;

    playbackBeat_.store((float)beatAtBlockStart, std::memory_order_relaxed);

    // Advance internal clock (DAW mode uses ppqPosition directly — no advance needed).
    if (!useDaw)
    {
        internalBeat_ += blockBeats;
        if (internalBeat_ >= loopLen)
            internalBeat_ = std::fmod(internalBeat_, loopLen);
    }

    // Auto-stop recording when loop length reached.
    // Called from the audio thread; recording_ is set false before finaliseRecording()
    // so no concurrent FIFO access occurs (recordGate/recordJoystick both early-return
    // when recording_=false, and we are the only process() invocation).
    if (recording_.load(std::memory_order_relaxed))
    {
        recordedBeats_ += blockBeats;
        if (recordedBeats_ >= loopLen)
        {
            recording_.store(false, std::memory_order_relaxed);
            finaliseRecording();       // merge FIFO → playbackStore_
            internalBeat_  = 0.0;     // restart at loop head (free-running mode)
            loopStartPpq_  = -1.0;    // re-anchor on next DAW play (DAW sync mode)
            recordedBeats_ = 0.0;
            // playing_ stays true — fall through to scan playbackStore_ this block
        }
    }

    // ── Scan playbackStore_ ───────────────────────────────────────────────────
    // playbackStore_ is audio-thread-only during playback.
    // The SPSC invariant ensures recording_=false when playing_=true in the sequential
    // model — punch-in requires stop before re-record, so no concurrent FIFO writer.
    const int count = playbackCount_.load(std::memory_order_acquire);
    for (int i = 0; i < count; ++i)
    {
        const auto& ev = playbackStore_[i];
        bool inWindow = wraps
            ? (ev.beatPosition >= beatAtBlockStart || ev.beatPosition < beatAtEnd)
            : (ev.beatPosition >= beatAtBlockStart && ev.beatPosition < beatAtEnd);

        if (!inWindow) continue;

        switch (ev.type)
        {
            case LooperEvent::Type::JoystickX:
                out.hasJoystickX = true;
                out.joystickX    = ev.value;
                break;
            case LooperEvent::Type::JoystickY:
                out.hasJoystickY = true;
                out.joystickY    = ev.value;
                break;
            case LooperEvent::Type::FilterX:
                out.hasFilterX = true;
                out.filterX    = ev.value;
                break;
            case LooperEvent::Type::FilterY:
                out.hasFilterY = true;
                out.filterY    = ev.value;
                break;
            case LooperEvent::Type::Gate:
                if (ev.voice >= 0 && ev.voice < 4)
                {
                    if (ev.value > 0.5f)
                        out.gateOn[ev.voice]  = true;
                    else
                        out.gateOff[ev.voice] = true;
                }
                break;
        }
    }

    return out;
}
