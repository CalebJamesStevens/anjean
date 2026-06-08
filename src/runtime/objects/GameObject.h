#pragma once

#include <cstdint>
#include <optional>

#include "../RuntimeTypes.h"
#include "Texture.h"
#include "Collider.h"

namespace Anjean::Runtime
{
    class GameObject
    {
      public:
        virtual ~GameObject() = default;

        std::uint32_t id = 0;

        Transform transform;

        std::optional<Mesh> mesh = std::nullopt;
        std::optional<Texture> texture = std::nullopt;
        std::optional<std::uint32_t> physicsBodyId;
        std::optional<Collider> collider = std::nullopt;

        virtual GameObjectType getGameObjectType() const
        {
            return ANJEAN_GAMEOBJECT;
        }
    };
}