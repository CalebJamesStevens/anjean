#pragma once

#include <SDL3/SDL.h>
#include <memory>

#include "../../rendering/RenderTypes.h"

namespace Anjean
{

    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Transform 
    {
        Vec3 position;
        Vec3 rotation;
        Vec3 scale = { 1.0f, 1.0f, 1.0f };
    };

    class GameObject final
    {
      public:
        Transform transform;
        Mesh mesh;
        ObjectUniformHandle objectUniformHandle;
    };
}