#include "SdlContext.h"
#include <SDL.h>
#include <atomic>

static std::atomic<int>  gRefCount  { 0 };
static std::atomic<bool> gAvailable { false };

bool SdlContext::acquire()
{
    if (gRefCount.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        // First caller — set hints then initialise SDL.
        // These MUST be set before SDL_Init so SDL uses its background
        // joystick thread instead of requiring a Win32 message pump
        // on the calling (plugin) thread.
        SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

        const bool ok = (SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
        if (!ok)
            DBG("SdlContext: SDL_Init failed: " << SDL_GetError());
        gAvailable.store(ok, std::memory_order_release);
        return ok;
    }
    // Subsequent callers — SDL already initialised.
    return gAvailable.load(std::memory_order_acquire);
}

void SdlContext::release()
{
    if (gRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        // Last caller — shut SDL down.
        SDL_Quit();
        gAvailable.store(false, std::memory_order_release);
    }
}

bool SdlContext::isAvailable()
{
    return gAvailable.load(std::memory_order_acquire);
}
