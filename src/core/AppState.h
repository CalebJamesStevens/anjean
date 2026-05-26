#pragma once

#include <SDL3/SDL.h>
#include <memory>

namespace Anjean
{
    class Window;
    class Renderer;

    struct AppState
    {
        Uint64 last_step = 0;

        std::unique_ptr<Window> window;
        std::unique_ptr<Renderer> renderer;
    };
}