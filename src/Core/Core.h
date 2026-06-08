#pragma once
#include <iostream>
#include <cstdint>

namespace Anjean::Core
{
    struct Vector3
    {
        float x = 0;
        float y = 0;
        float z = 0;

        Vector3& operator+=(const Vector3& other)
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        Vector3& operator-=(const Vector3& other)
        {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        Vector3& operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        Vector3& operator/=(float scalar)
        {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            return *this;
        }
    };

    inline Vector3 operator+(Vector3 lhs, const Vector3& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    inline Vector3 operator-(Vector3 lhs, const Vector3& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    inline Vector3 operator*(Vector3 v, float scalar)
    {
        v *= scalar;
        return v;
    }

    inline Vector3 operator*(float scalar, Vector3 v)
    {
        v *= scalar;
        return v;
    }

    inline Vector3 operator/(Vector3 v, float scalar)
    {
        v /= scalar;
        return v;
    }
    struct Position3D
    {
        float x;
        float y;
        float z;
    };

    struct Position2D
    {
        float x;
        float y;
    };

    struct MeshVertex
    {
        Position3D position;
        Position2D uv;
    };

    struct MeshData
    {
        std::uint32_t id = 0;
        std::vector<MeshVertex> vertices;
    };

    struct Quaternion {
    float w, x, y, z;

    // Construct from axis-angle: q = [cos(θ/2), sin(θ/2) * axis]
    static Quaternion fromAxisAngle(float angleRadians, const Vector3& axis) {
        float halfAngle = angleRadians * 0.5f;
        float sinHalf = std::sin(halfAngle);
        return {
            std::cos(halfAngle),
            axis.x * sinHalf,
            axis.y * sinHalf,
            axis.z * sinHalf
        };
    }

    // Hamilton Product (Quaternion * Quaternion)
    Quaternion multiply(const Quaternion& q) const {
        return {
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        };
    }
};

    enum class ColliderType : std::uint32_t
  {
      Capsule = 0,
      RectangularPrism = 1,
      Sphere = 2,
      Plane = 3
  };

  enum class PhysicsBodyType : std::uint32_t
  {
      Static = 0,
      Kinematic = 1,
      Dynamic = 2
  };

  using ObjectId = std::uint32_t;

  struct ColliderState {
    ObjectId id = 0;
    Vector3 localCenter{};

    // Sphere
    float radius = 1.0f;

    // Rectangular prism
    Vector3 halfExtents{0.5f, 0.5f, 0.5f};
    ColliderType colliderType;
  };

  struct PhysicsBodyState
  {
      ObjectId id = 0;

      Vector3 position;
      Vector3 velocity;
      Vector3 force;
      PhysicsBodyType physicsBodyType;
      std::vector<ColliderState> colliders;

      float mass = 1.0f;
  };

  struct PhysicsBodyResult
  {
      ObjectId id = 0;

      Vector3 position;
      Vector3 velocity;
  };
}