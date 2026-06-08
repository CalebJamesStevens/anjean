#pragma once

#include "Collider.h"
#include "../../Core/Core.h"

namespace Anjean::Runtime {
  class SphereCollider : public Collider
  {
    public:
      SphereCollider()
      {
        Type = ColliderType::Sphere;
      }

      Anjean::Core::Vector3 Center;
      float Radius = 1.0f;
  };
}