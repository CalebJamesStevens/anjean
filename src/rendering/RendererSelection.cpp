#include "RendererSelection.h"

#include <stdexcept>

namespace Anjean::Rendering
{
GraphicsAPI ResolveGraphicsAPI(GraphicsAPI requested)
{
	if (requested != GraphicsAPI::Auto)
	{
		return requested;
	}

#if defined(ANJEAN_RENDERER_VULKAN) && !defined(ANJEAN_RENDERER_METAL)
	return GraphicsAPI::Vulkan;
#elif defined(ANJEAN_RENDERER_METAL) && !defined(ANJEAN_RENDERER_VULKAN)
	return GraphicsAPI::Metal;
#elif defined(ANJEAN_RENDERER_VULKAN) && defined(ANJEAN_RENDERER_METAL)
	return GraphicsAPI::Vulkan;        // your current preferred default while porting
#else
	throw std::runtime_error("No renderer backend was compiled.");
#endif
}

SDL_WindowFlags GetSDLWindowFlagsForGraphicsAPI(GraphicsAPI api)
{
	switch (api)
	{
		case GraphicsAPI::Vulkan:
			return SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;

		case GraphicsAPI::Metal:
			return SDL_WINDOW_RESIZABLE | SDL_WINDOW_METAL;

		case GraphicsAPI::Auto:
			throw std::runtime_error("GraphicsAPI::Auto must be resolved before creating the window.");
	}

	throw std::runtime_error("Unknown graphics API.");
}
}        // namespace Anjean::Rendering