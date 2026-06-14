#pragma once
#include <cstdint>
#include <iostream>

#include "Core.h"

namespace Anjean::Core::Math {
  inline float dotProduct(const Vector3& a, const Vector3& b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
  }
  inline bool SameDirection(const Vector3& direction, const Vector3& ao) {
    return dotProduct(direction, ao) > 0;
  }
  inline Vector3 crossProduct(const Vector3& a, const Vector3& b) {
    return Vector3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
  }
  inline bool Line(std::vector<Vector3>& points, Vector3& direction) {
    Vector3 a = points[0];
    Vector3 b = points[1];

    Vector3 ab = b - a;
    Vector3 ao = Vector3{0, 0, 0} - a;

    if (SameDirection(ab, ao)) {
      direction = crossProduct(crossProduct(ab, ao), ab);
    }

    else {
      points = {a};
      direction = ao;
    }

    return false;
  }
  inline bool Triangle(std::vector<Vector3>& points, Vector3& direction) {
    Vector3 a = points[0];
    Vector3 b = points[1];
    Vector3 c = points[2];

    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 ao = Vector3{0, 0, 0} - a;

    Vector3 abc = crossProduct(ab, ac);

    if (SameDirection(crossProduct(abc, ac), ao)) {
      if (SameDirection(ac, ao)) {
        points = {a, c};
        direction = crossProduct(crossProduct(ac, ao), ac);
      }

      else {
        return Line(points = {a, b}, direction);
      }
    }

    else {
      if (SameDirection(crossProduct(ab, abc), ao)) {
        return Line(points = {a, b}, direction);
      }

      else {
        if (SameDirection(abc, ao)) {
          direction = abc;
        }

        else {
          points = {a, c, b};
          direction = Vector3{0, 0, 0} - abc;
        }
      }
    }

    return false;
  }
  inline bool Tetrahedron(std::vector<Vector3>& points, Vector3& direction) {
    Vector3 a = points[0];
    Vector3 b = points[1];
    Vector3 c = points[2];
    Vector3 d = points[3];

    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 ad = d - a;
    Vector3 ao = Vector3{0, 0, 0} - a;

    Vector3 abc = crossProduct(ab, ac);
    Vector3 acd = crossProduct(ac, ad);
    Vector3 adb = crossProduct(ad, ab);

    if (SameDirection(abc, ao)) {
      return Triangle(points = {a, b, c}, direction);
    }

    if (SameDirection(acd, ao)) {
      return Triangle(points = {a, c, d}, direction);
    }

    if (SameDirection(adb, ao)) {
      return Triangle(points = {a, d, b}, direction);
    }

    return true;
  }
  inline float Length(const Vector3& v) {
    return std::sqrt(dotProduct(v, v));
  }
  inline Vector3 Vector3Normalize(const Vector3& v) {
    float len = Length(v);
    if (len <= 0.000001f)
      return Vector3{0, 0, 0};
    return v / len;
  }

} // namespace Anjean::Core::Math