#pragma once

#include <SDL3/SDL.h>
#include <memory>

#include "../rendering/RenderTypes.h"

namespace Anjean {
  class Window;
  class Renderer;
}

namespace Anjean::Core
{
    class GameObject;

    struct AppState
    {
        Uint64 last_step = 0;

        std::unique_ptr<Anjean::Window> window;
        std::unique_ptr<Anjean::Renderer> renderer;

        PipelineHandle basicColorPipeline;
        TextureHandle demoTexture;
        Mesh demoMesh;
        Mesh demoMesh2;
        float rotationSpeed;
        simd_float3 cameraPos;
        simd_float4x4 cameraMatrix;
        simd_float4x4 projectionMatrix;
        std::unique_ptr<GameObject> camera;
        std::unique_ptr<GameObject> cube1;
        std::unique_ptr<GameObject> cube2;
        std::vector<GameObject*> gameObjectsToRender;
    };
}