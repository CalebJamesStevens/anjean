#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>

#include "../RuntimeTypes.h"
#include "Texture.h"

namespace Anjean::Runtime
{
    class GameObject
    {
      public:
        uint32_t id;
        Transform transform;
        std::optional<Mesh> mesh = std::nullopt;
        std::optional<Texture> texture = std::nullopt;

        virtual GameObjectType getGameObjectType(){
          return ANJEAN_GAMEOBJECT;
        };
    };
}