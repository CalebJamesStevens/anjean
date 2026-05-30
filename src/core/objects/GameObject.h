#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>

#include "../../rendering/RenderTypes.h"
#include "CoreTypes.h"

namespace Anjean::Core
{
    class GameObject
    {
      public:
        Transform transform;
        Mesh mesh;
        virtual GameObjectType getGameObjectType(){
          return ANJEAN_GAMEOBJECT;
        };
    };
}