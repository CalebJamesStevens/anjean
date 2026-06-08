#pragma once

#include "Collider.h"
#include "../../Core/Core.h"

namespace Anjean::Runtime {
  class RectangularPrismCollider : public Collider
  {
    public:
      RectangularPrismCollider()
      {
        Type = ColliderType::RectangularPrism;
      }
  };
}