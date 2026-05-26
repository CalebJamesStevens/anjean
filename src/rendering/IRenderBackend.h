#pragma once

#include "RenderTypes.h"

namespace Anjean
{
    class IRenderBackend
    {
    public:
        virtual ~IRenderBackend() = default;

        virtual void beginFrame(const Color& clearColor) = 0;
        virtual void drawTestTriangle() = 0;
        virtual void endFrame() = 0;
    };
}