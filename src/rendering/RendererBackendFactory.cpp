#include "RendererBackendFactory.h"

#include "IRenderBackend.h"
#include "RendererSelection.h"
#include <memory>
#include <stdexcept>

#if defined(ANJEAN_RENDERER_METAL)
#	include "backends/metal/MetalRendererBackend.h"
#endif

#if defined(ANJEAN_RENDERER_VULKAN)
#	include "backends/vulkan/VulkanRendererBackend.h"
#endif

namespace Anjean::Rendering
{
std::unique_ptr<IRenderBackend> CreateRendererBackend(
    Anjean::Window       &window,
    const RendererConfig &config)
{
	GraphicsAPI api = ResolveGraphicsAPI(config.api);

	switch (api)
	{
		case GraphicsAPI::Metal:
#if defined(ANJEAN_RENDERER_METAL)
			return std::make_unique<MetalRendererBackend>(window);
#else
			throw std::runtime_error("Metal renderer was requested but was not compiled.");
#endif

		case GraphicsAPI::Vulkan:
#if defined(ANJEAN_RENDERER_VULKAN)
			return std::make_unique<VulkanRendererBackend>(window);
#else
			throw std::runtime_error("Vulkan renderer was requested but was not compiled.");
#endif

		case GraphicsAPI::Auto:
			break;
	}

	throw std::runtime_error("No valid renderer backend selected.");
}
}        // namespace Anjean::Rendering