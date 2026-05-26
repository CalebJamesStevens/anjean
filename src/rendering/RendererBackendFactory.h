#pragma once

#include <memory>

#include "RenderTypes.h"

namespace Anjean
{
    class Window;
    class IRenderBackend;

    std::unique_ptr<IRenderBackend> CreateRendererBackend(
        Window& window,
        const RendererConfig& config
    );
}