#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <vector>

namespace Anjean {
  class Window {
  public:
    Window(const char* title, int width, int height, SDL_WindowFlags flags);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    SDL_Window* getNativeWindow() const;
    float width = 0;
    float height = 0;
    std::vector<const char*> getRequiredVulkanExtensions() const;
    VkSurfaceKHR createVulkanSurface(VkInstance instance) const;

  private:
    SDL_Window* m_window = nullptr;
  };
} // namespace Anjean