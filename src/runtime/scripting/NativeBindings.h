#pragma once

#include <cstdint>
#include "../RuntimeTypes.h"


namespace Anjean::Runtime
{
    class Runtime;

    void BindNativeRuntime(Runtime* runtime);
}

extern "C"
{
    struct AnjeanVec3
    {
        float x;
        float y;
        float z;
    };

    int Anjean_GameObject_GetPosition(
        std::uint32_t gameObjectId,
        AnjeanVec3* outPosition
    );

    int Anjean_GameObject_SetPosition(
        std::uint32_t gameObjectId,
        AnjeanVec3 position
    );

    int Anjean_GameObject_SetTexture(
      std::uint32_t gameObjectId,
      const char* filename,
      std::uint32_t width,
      std::uint32_t height,
      std::uint32_t channels
    );
    
    int Anjean_GameObject_SetMesh(
      std::uint32_t gameObjectId,
      std::uint32_t meshId
    );
    
    int Anjean_Input_IsKeyDown(int KeyCode);
}