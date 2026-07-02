#pragma once

#include "RenderTypes.h"        // use the actual name of the file you pasted

#include <SDL3/SDL_video.h>

namespace Anjean::Rendering
{
GraphicsAPI ResolveGraphicsAPI(GraphicsAPI requested);

SDL_WindowFlags GetSDLWindowFlagsForGraphicsAPI(GraphicsAPI api);
}        // namespace Anjean::Rendering