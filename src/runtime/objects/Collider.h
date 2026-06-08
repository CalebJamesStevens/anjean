#pragma once

#include "../RuntimeTypes.h"

namespace Anjean::Runtime {
  class Collider
  {
    public:
      std::uint32_t id = 0;
      Core::ColliderType type = Core::ColliderType::RectangularPrism;

      Core::Vector3 localCenter{};

      // Sphere
      float radius = 1.0f;

      // Rectangular prism
      Core::Vector3 halfExtents{0.5f, 0.5f, 0.5f};
  };
}