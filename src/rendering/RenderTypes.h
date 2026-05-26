#pragma once

namespace Anjean
{
    struct Color
    {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 1.0f;
    };

    struct Rect
    {
        int x, y, w, h;
    };

    enum class GraphicsAPI
    {
        Auto,
        Metal,
        Vulkan
    };

    struct RendererConfig
    {
        GraphicsAPI api = GraphicsAPI::Auto;
    };
}