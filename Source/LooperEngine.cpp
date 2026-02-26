#include "LooperEngine.h"
#include <cmath>
#include <algorithm>

// No #include <mutex> — this file contains zero blocking calls.

// ─── Quantize math utilities ─────────────────────────────────────────────────
// Free functions declared in LooperEngine.h for testability.

double snapToGrid(double beatPos, double gridSize, double loopLen) noexcept
{
    if (gridSize <= 0.0 || loopLen <= 0.0) return beatPos;
    const double lower    = std::floor(beatPos / gridSize) * gridSize;
    const double upper    = lower + gridSize;
    const double midpoint = lower + gridSize * 0.5;
    // Ties snap to the EARLIER grid point: beatPos == midpoint → lower (earlier).
    // Condition: (beatPos <= midpoint) → lower; (beatPos > midpoint) → upper.
    const double snapped  = (beatPos <= midpoint) ? lower : upper;
    return std::fmod(snapped, loopLen);  // mandatory: fmod wraps snapped==loopLen → 0.0
}

double quantizeSubdivToGridSize(int subdivIdx) noexcept
{
    switch (subdivIdx)
    {
        case 0: return 1.0;    // 1/4 note
        case 1: return 0.5;    // 1/8 note
        case 2: return 0.25;   // 1/16 note
        case 3: return 0.125;  // 1/32 note
        default: return 0.5;   // safe fallback: 1/8
    }
}

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

    // DAW sync mode: PLAY button can stop the looper but never starts it
    // (DAW transport controls start). When stopped, press is a no-op — the
    // button blinks in the UI to show "armed, waiting for DAW play".
    if (syncToDaw_.load())
    {
        if (wasPlaying)
        {
            playing_.store(false);
            recPendingNextCycle_.store(false);
            recording_.store(false);
            finaliseRecording();
        }
        return;
    }

    if (!wasPlaying)
    {
        playing_.store(true);
    }
    else
    {
        // Stop: cancel any pending overdub arm, freeze FIFO into playbackStore_.
        // INVARIANT: recording_ must be false before finaliseRecording().
        playing_.store(false);
        recPendingNextCycle_.store(false);
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

    // Cancel whichever pending arm is active (second press = cancel).
    if (recordPending_.load())       { recordPending_.store(false);       return; }
    if (recPendingNextCycle_.load()) { recPendingNextCycle_.store(false);  return; }
    if (recWaitArmed_.load())        { recWaitArmed_.store(false);         return; }

    // REC no longer implies PLAY — user starts the loop separately with PLAY.
    // recordPending_ is serviced by process() once playing_ becomes true.

    if (recWaitForTrigger_.load())
    {
        recWaitArmed_.store(true);
        return;
    }

    // If loop is already playing and has content: defer start to next cycle boundary.
    if (playing_.load() && playbackCount_.load(std::memory_order_relaxed) > 0)
    {
        recPendingNextCycle_.store(true);
        return;
    }

    // Otherwise arm immediately; process() will activate on next valid clock.
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

// ─── armWait (message-thread) ─────────────────────────────────────────────────

void LooperEngine::armWait()
{
    // Cancel if already waiting.
    if (recWaitArmed_.load())
    {
        recWaitArmed_.store(false);
        return;
    }

    // Do NOT start playing yet — play+record both start when the trigger fires.
    // The PLAY button blinks in the UI to signal "waiting for touch".
    recWaitArmed_.store(true);
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
    // Reset beat so recording always starts from the beginning of the loop.
    internalBeat_ = 0.0;
    loopStartPpq_ = -1.0;  // re-anchor to DAW on next play (DAW sync mode)
    // Start play and record simultaneously on the trigger.
    playing_.store(true, std::memory_order_relaxed);
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
    // IMPORTANT: only discard an old event if a new event of the SAME type (and same
    // voice for Gate events) falls within the touch radius. This prevents joystick
    // recordings from accidentally wiping gate events that share a nearby beat position.
    const int oldCount = playbackCount_.load(std::memory_order_relaxed);
    for (int i = 0; i < oldCount && mergedCount < LOOPER_FIFO_CAPACITY; ++i)
    {
        const auto& oldEv  = playbackStore_[i];
        const double oldPos = oldEv.beatPosition;
        bool inTouchedWindow = false;
        for (int j = 0; j < newEventCount; ++j)
        {
            const auto& newEv = scratchNew_[j];
            // Type mismatch → new event cannot replace old event.
            if (newEv.type != oldEv.type) continue;
            // For Gate events, voice must also match (don't let voice-0 gate erase voice-1).
            if (newEv.type == LooperEvent::Type::Gate && newEv.voice != oldEv.voice) continue;
            const double dist = std::abs(oldPos - newEv.beatPosition);
            // Handle wrap-around at loop boundary
            const double distWrapped = std::min(dist, loopLen - dist);
            if (distWrapped <= touchRadius)
            {
                inTouchedWindow = true;
                break;
            }
        }
        if (!inTouchedWindow)
            scratchMerged_[mergedCount++] = oldEv;
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

    // Sort playbackStore_ by beatPosition so events always fire in chronological
    // order. After punch-in merge, surviving old events and appended new events
    // are interleaved by time but not by array index — sorting fixes out-of-order
    // FilterX/FilterY (and any other) events that would otherwise play back wrong.
    {
        const int n = playbackCount_.load(std::memory_order_relaxed);
        std::sort(playbackStore_.begin(), playbackStore_.begin() + n,
                  [](const LooperEvent& a, const LooperEvent& b) {
                      return a.beatPosition < b.beatPosition;
                  });
    }

    // Update content flags so the processor knows which axes have recorded material.
    // This lets it allow the live stick when nothing has been recorded yet (no jumps).
    {
        bool hasFilter = false, hasJoy = false;
        const int n = playbackCount_.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            const auto t = playbackStore_[i].type;
            if (t == LooperEvent::Type::FilterX   || t == LooperEvent::Type::FilterY)   hasFilter = true;
            if (t == LooperEvent::Type::JoystickX || t == LooperEvent::Type::JoystickY) hasJoy    = true;
            if (hasFilter && hasJoy) break;
        }
        hasFilterContent_.store  (hasFilter, std::memory_order_relaxed);
        hasJoystickContent_.store(hasJoy,    std::memory_order_relaxed);
    }

    // Reset shadow copy: old originals are stale after new recording.
    hasOriginals_.store(false, std::memory_order_relaxed);
    quantizeActive_.store(false, std::memory_order_relaxed);
    // If Post mode is still active, re-apply quantize to the new content on next process() call.
    if (quantizeMode_.load(std::memory_order_relaxed) == 2 /*kQuantizePost*/)
        pendingQuantize_.store(true, std::memory_order_release);
}

// ─── Quantize helpers ─────────────────────────────────────────────────────────

void LooperEngine::applyQuantizeToStore(double gridSize, double loopLen)
{
    const int n = playbackCount_.load(std::memory_order_relaxed);
    if (n == 0) return;

    // Step 1: snap all Gate events; leave Joystick/Filter events untouched.
    // Track per-voice last original note-on position for duration-preserving note-off.
    double lastOriginalOnBeat[4] = { -1.0, -1.0, -1.0, -1.0 };  // -1 = no pending note-on

    for (int i = 0; i < n; ++i)
    {
        auto& ev = playbackStore_[i];
        if (ev.type != LooperEvent::Type::Gate) continue;

        const bool isOn = (ev.value > 0.5f);

        if (isOn)
        {
            const double original = ev.beatPosition;
            lastOriginalOnBeat[ev.voice] = original;
            ev.beatPosition = snapToGrid(original, gridSize, loopLen);
        }
        else
        {
            // Duration-preserving shift for note-off
            if (lastOriginalOnBeat[ev.voice] >= 0.0)
            {
                double duration = ev.beatPosition - lastOriginalOnBeat[ev.voice];
                if (duration < 0.0) duration += loopLen;  // wrap case
                const double minGate = 1.0 / 64.0;
                duration = std::max(duration, minGate);
                const double snappedOn = snapToGrid(lastOriginalOnBeat[ev.voice], gridSize, loopLen);
                ev.beatPosition = std::fmod(snappedOn + duration, loopLen);
                lastOriginalOnBeat[ev.voice] = -1.0;  // reset tracking
            }
            // If no matching note-on tracked, leave note-off beat unchanged
        }
    }

    // Step 2: sort by beatPosition (matches finaliseRecording sort behavior)
    std::sort(playbackStore_.begin(), playbackStore_.begin() + n,
              [](const LooperEvent& a, const LooperEvent& b) {
                  return a.beatPosition < b.beatPosition;
              });

    // Step 3: deduplicate — same voice + same snapped beat Gate-on -> keep first, discard later
    // Use scratchDedup_ class member (avoids ~49KB stack allocation).
    int writeIdx = 0;
    for (int i = 0; i < n; ++i)
    {
        const auto& ev = playbackStore_[i];
        if (ev.type == LooperEvent::Type::Gate && ev.value > 0.5f)
        {
            // Check if we already have a gate-on for this voice at this beat position
            bool isDuplicate = false;
            for (int j = 0; j < writeIdx; ++j)
            {
                const auto& prev = scratchDedup_[j];
                if (prev.type == LooperEvent::Type::Gate && prev.value > 0.5f
                    && prev.voice == ev.voice
                    && std::abs(prev.beatPosition - ev.beatPosition) < 1e-9)
                {
                    isDuplicate = true;
                    break;
                }
            }
            if (isDuplicate) continue;
        }
        if (writeIdx < LOOPER_FIFO_CAPACITY)
            scratchDedup_[writeIdx++] = ev;
    }

    // Copy deduped back to playbackStore_
    for (int i = 0; i < writeIdx; ++i)
        playbackStore_[i] = scratchDedup_[i];
    playbackCount_.store(writeIdx, std::memory_order_release);
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

    double pos = std::fmod(beatPos, loopLen);

    // Live quantize: snap gate-on; shift gate-off as rigid unit (duration preserved)
    if (quantizeMode_.load(std::memory_order_relaxed) == 1 /*kQuantizeLive*/)
    {
        const double grid = quantizeSubdivToGridSize(
            quantizeSubdiv_.load(std::memory_order_relaxed));
        if (on)
        {
            lastRawOnBeat_[voice] = pos;           // save raw position BEFORE snap
            pos = snapToGrid(pos, grid, loopLen);
            lastSnappedOnBeat_[voice] = pos;
        }
        else
        {
            // Recover original duration from raw positions (handles loop wrap)
            const double rawPos = std::fmod(beatPos, loopLen);
            double duration = rawPos - lastRawOnBeat_[voice];
            if (duration < 0.0) duration += loopLen;
            const double minGate = 1.0 / 64.0;
            duration = std::max(duration, minGate);
            pos = std::fmod(lastSnappedOnBeat_[voice] + duration, loopLen);
        }
    }

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
        // Seek to loop start: keep content and play state.
        // If recording was in progress, discard the in-flight FIFO without finalising.
        if (recording_.load(std::memory_order_relaxed))
        {
            recording_.store(false, std::memory_order_relaxed);
            fifo_.reset();
        }
        internalBeat_  = 0.0;
        loopStartPpq_  = -1.0;
        playbackBeat_.store(0.0f, std::memory_order_relaxed);
        out.looperReset = true;  // caller sends note-offs for any active looper pitches
        return out;
    }

    if (deleteRequest_.exchange(false, std::memory_order_acq_rel))
    {
        fifo_.reset();
        playbackStore_ = {};
        playbackCount_.store(0,     std::memory_order_relaxed);
        hasFilterContent_.store  (false, std::memory_order_relaxed);
        hasJoystickContent_.store(false, std::memory_order_relaxed);
        quantizeActive_.store(false, std::memory_order_relaxed);
        hasOriginals_.store(false,   std::memory_order_relaxed);
        return out;
    }

    // ── Post-record quantize service ──────────────────────────────────────────
    if (pendingQuantize_.exchange(false, std::memory_order_acq_rel))
    {
        // Guard: never apply while recording (UI disables button but guard defensively)
        if (!recording_.load(std::memory_order_relaxed))
        {
            const int n = playbackCount_.load(std::memory_order_relaxed);
            const double loopLen = getLoopLengthBeats();
            const double grid = quantizeSubdivToGridSize(
                quantizeSubdiv_.load(std::memory_order_relaxed));

            // Save originals to shadow copy for revert
            for (int i = 0; i < n; ++i)
                originalStore_[i] = playbackStore_[i];
            originalCount_.store(n, std::memory_order_relaxed);
            hasOriginals_.store(true, std::memory_order_relaxed);

            applyQuantizeToStore(grid, loopLen);
            quantizeActive_.store(true, std::memory_order_relaxed);
        }
    }

    if (pendingQuantizeRevert_.exchange(false, std::memory_order_acq_rel))
    {
        if (hasOriginals_.load(std::memory_order_relaxed))
        {
            const int n = originalCount_.load(std::memory_order_relaxed);
            for (int i = 0; i < n; ++i)
                playbackStore_[i] = originalStore_[i];
            playbackCount_.store(n, std::memory_order_release);
            quantizeActive_.store(false, std::memory_order_relaxed);
        }
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
                // DAW just started → always start looper when sync is on.
                // REC armed state (recordPending_) will activate at the next valid clock.
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

    // Activate overdub that was deferred to the next cycle boundary.
    if (recPendingNextCycle_.load(std::memory_order_relaxed) && wraps)
    {
        recPendingNextCycle_.store(false, std::memory_order_relaxed);
        capReached_.store(false, std::memory_order_relaxed);
        fifo_.reset();
        recording_.store(true, std::memory_order_relaxed);
        recordedBeats_ = 0.0;
    }

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
