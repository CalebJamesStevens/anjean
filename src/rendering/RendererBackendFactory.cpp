#include "RendererBackendFactory.h"

#include "IRenderBackend.h"

#include <stdexcept>

#if defined(ANJEAN_RENDERER_METAL)
    #include "backends/metal/MetalRendererBackend.h"
#endif

#if defined(ANJEAN_RENDERER_VULKAN)
    #include "backends/vulkan/VulkanRendererBackend.h"
#endif

namespace Anjean
{
    std::unique_ptr<IRenderBackend> CreateRendererBackend(
        Window& window,
        const RendererConfig& config
    )
    {
        GraphicsAPI api = config.api;

        if (api == GraphicsAPI::Auto)
        {
#if defined(ANJEAN_PLATFORM_MACOS)
            api = GraphicsAPI::Metal;
#else
            api = GraphicsAPI::Vulkan;
#endif
        }

        switch (api)
        {
            case GraphicsAPI::Metal:
#if defined(ANJEAN_RENDERER_METAL)
                return std::make_unique<MetalRendererBackend>(window);
#else
                throw std::runtime_error("Metal renderer was not compiled for this platform.");
#endif

            case GraphicsAPI::Vulkan:
#if defined(ANJEAN_RENDERER_VULKAN)
                return std::make_unique<VulkanRendererBackend>(window);
#else
                throw std::runtime_error("Vulkan renderer was not compiled for this platform.");
#endif

            case GraphicsAPI::Auto:
                break;
        }

        throw std::runtime_error("No valid renderer backend selected.");
    }
}