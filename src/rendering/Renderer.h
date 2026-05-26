#pragma once

#include <memory>

#include "RenderTypes.h"

namespace Anjean
{
    class Window;
    class IRenderBackend;

    class Renderer
    {
    public:
        explicit Renderer(Window& window, const RendererConfig& config = {});
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        Renderer(Renderer&&) noexcept = default;
        Renderer& operator=(Renderer&&) noexcept = default;

        void beginFrame(const Color& clearColor);
        void endFrame();
        bool renderRect(Renderer*renderer, const Rect *rect);
        void drawTestTriangle();

    private:
        std::unique_ptr<IRenderBackend> m_backend;
    };
}