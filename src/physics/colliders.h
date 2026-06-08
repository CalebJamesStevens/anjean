#include "../Core/Core.h"

namespace Anjean::Physics {
  enum ColliderType {
    SPHERE,
    PLANE
  };
  
  struct Collider {
    ColliderType Type;
  };
  
  struct SphereCollider : Collider {
    Core::Vector3 Center;
    float Radius;
  };
  
  struct PlaneCollider : Collider {
    Core::Vector3 Normal;
    float Distance;
  };
}