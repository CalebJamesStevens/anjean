#pragma once

#include "../../IRenderBackend.h"

#include <memory>

namespace Anjean
{
    class Window;

    class MetalRendererBackend final : public IRenderBackend
    {
    public:
        explicit MetalRendererBackend(Window& window);
        ~MetalRendererBackend() override;

        MetalRendererBackend(const MetalRendererBackend&) = delete;
        MetalRendererBackend& operator=(const MetalRendererBackend&) = delete;

        MetalRendererBackend(MetalRendererBackend&&) noexcept = delete;
        MetalRendererBackend& operator=(MetalRendererBackend&&) noexcept = delete;

        void beginFrame(const Color& clearColor) override;
        void drawTestTriangle() override;
        void endFrame() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}