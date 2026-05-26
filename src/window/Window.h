#pragma once

#include <SDL3/SDL.h>

namespace Anjean
{
    class Window
    {
    public:
        Window(const char* title, int width, int height, SDL_WindowFlags flags);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        SDL_Window* getNativeWindow() const;

    private:
        SDL_Window* m_window = nullptr;
    };
}