#pragma once

#include <cstdint>
#include <vector>

#include "../RuntimeTypes.h"
#include "Collider.h"

namespace Anjean::Runtime
{
    class PhysicsBody
    {
    public:
        std::uint32_t id = 0;
        std::uint32_t nextColliderId = 1;

        Core::PhysicsBodyType type = Core::PhysicsBodyType::Kinematic;

        Core::Vector3 velocity{};
        Core::Vector3 force{};
        float mass = 1.0f;

        std::vector<Collider> colliders;

        Core::PhysicsBodyType getPhysicsBodyType() const
        {
            return type;
        }
    };
}