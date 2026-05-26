#include "Window.h"

#include <stdexcept>
#include <string>

namespace Anjean
{
    Window::Window(const char* title, int width, int height, SDL_WindowFlags flags)
    {
        m_window = SDL_CreateWindow(title, width, height, flags);

        if (m_window == nullptr)
        {
            throw std::runtime_error(
                std::string("Failed to create SDL window: ") + SDL_GetError()
            );
        }
    }

    Window::~Window()
    {
        if (m_window != nullptr)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    Window::Window(Window&& other) noexcept
        : m_window(other.m_window)
    {
        other.m_window = nullptr;
    }

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this != &other)
        {
            if (m_window != nullptr)
            {
                SDL_DestroyWindow(m_window);
            }

            m_window = other.m_window;
            other.m_window = nullptr;
        }

        return *this;
    }

    SDL_Window* Window::getNativeWindow() const
    {
        return m_window;
    }
}