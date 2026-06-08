#include "../Core/Core.h"

namespace Anjean::Physics {
  struct CollisionPoints {
    Core::Vector3 A; // Furthest point of A into B
    Core::Vector3 B; // Furthest point of B into A
    Core::Vector3 Normal; // B – A normalized
    float Depth;    // Length of B – A
    bool HasCollision;
  };
   
  struct Transform { // Describes an objects location
    Core::Vector3 Position;
    Core::Vector3 Scale;
    Core::Quaternion Rotation;
  };
}