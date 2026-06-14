#pragma once

#include "../Runtime.h"

/*
  Inspired partially by Jolt
  You provide a desired velocity, usually from player input plus gravity.
  Jolt checks what the character is touching or near.
  It turns contacts into movement constraints.
  It solves those constraints to produce a safe displacement.
  It sweeps along that displacement to catch missed collisions.
  It updates position, active contacts, ground state, and optional stair/floor behavior.

  ExtendedUpdate()
  -> Update()
    -> MoveShape()
      -> GetContactsAtPosition()
      -> RemoveConflictingContacts()
      -> DetermineConstraints()
      -> SolveConstraints()
      -> GetFirstContactForSweep()
      -> apply displacement
    -> UpdateSupportingContact()
    -> UpdateInnerBodyTransform()
*/

namespace Anjean::Runtime {
  class CharacterController {
    struct SupportPoint {
      Core::Vector3 a; // point on shape A
      Core::Vector3 b; // point on shape B
      Core::Vector3 v; // a - b
    };
    struct Manifold {
      bool hasCollision = false;
      Core::Vector3 normal{};
      float depth = 0.0f;
    };
    struct SweepHit {
      bool hasHit = false;
      float t = 1.0f;
      Core::Vector3 normal{};
      float depth = 0.0f;
    };
    struct GJKDistanceResult {
      bool overlapped = false;
      float distance = 0.0f;

      // Normal from A to B.
      Core::Vector3 normal{};

      Core::Vector3 closestA{};
      Core::Vector3 closestB{};

      // Use this if overlapped and you want to call EPA.
      std::vector<Core::Vector3> epaSimplex{};
    };
    struct ClosestResult {
      bool containsOrigin = false;

      Core::Vector3 closest{};
      Core::Vector3 closestA{};
      Core::Vector3 closestB{};
    };
    struct EPAFace {
      size_t a, b, c;
      Core::Vector3 normal;
      float distance;
    };

  public:
    Core::PhysicsBodyState body;
    std::uint32_t id = 0;
    Core::ColliderType type = Core::ColliderType::RectangularPrism;
    Core::Vector3 localCenter{};

    // Sphere
    float radius = 1.0f;

    // Rectangular prism
    Core::Vector3 halfExtents{0.5f, 0.5f, 0.5f};
    bool ProcessSimplex(std::vector<Core::Vector3>& points, Core::Vector3& direction) {
      switch (points.size()) {
      case 2:
        return Core::Math::Line(points, direction);
      case 3:
        return Core::Math::Triangle(points, direction);
      case 4:
        return Core::Math::Tetrahedron(points, direction);
      }
      return false;
    }

    Core::Vector3 Support(std::vector<Core::Vector3> m_vertsA, std::vector<Core::Vector3> m_vertsB,
                          Core::Vector3 direction) {
      float maxDistanceA = -INFINITY;
      float maxDistanceB = -INFINITY;
      Core::Vector3 maxPointA;
      Core::Vector3 maxPointB;
      for (Core::Vector3 vertex : m_vertsA) {
        float distance = Core::Math::dotProduct(vertex, direction);
        if (distance > maxDistanceA) {
          maxDistanceA = distance;
          maxPointA = vertex;
        }
      }

      for (Core::Vector3 vertex : m_vertsB) {
        float distance = Core::Math::dotProduct(vertex, direction * -1);
        if (distance > maxDistanceB) {
          maxDistanceB = distance;
          maxPointB = vertex;
        }
      }

      return maxPointA - maxPointB;
    }
    SupportPoint SupportDistance(const std::vector<Core::Vector3>& vertsA,
                                 const std::vector<Core::Vector3>& vertsB,
                                 Core::Vector3 direction) {
      float maxDistanceA = -INFINITY;
      float maxDistanceB = -INFINITY;

      Core::Vector3 maxPointA{};
      Core::Vector3 maxPointB{};

      for (Core::Vector3 vertex : vertsA) {
        float distance = Core::Math::dotProduct(vertex, direction);
        if (distance > maxDistanceA) {
          maxDistanceA = distance;
          maxPointA = vertex;
        }
      }

      Core::Vector3 oppositeDirection = direction * -1.0f;

      for (Core::Vector3 vertex : vertsB) {
        float distance = Core::Math::dotProduct(vertex, oppositeDirection);
        if (distance > maxDistanceB) {
          maxDistanceB = distance;
          maxPointB = vertex;
        }
      }

      return SupportPoint{.a = maxPointA, .b = maxPointB, .v = maxPointA - maxPointB};
    }
    ClosestResult ClosestPointPoint(std::vector<SupportPoint>& simplex) {
      const SupportPoint& a = simplex[0];

      return ClosestResult{
          .containsOrigin = false, .closest = a.v, .closestA = a.a, .closestB = a.b};
    }
    ClosestResult ClosestPointLine(std::vector<SupportPoint>& simplex) {
      SupportPoint a = simplex[0];
      SupportPoint b = simplex[1];

      Core::Vector3 ab = b.v - a.v;

      float denom = Core::Math::dotProduct(ab, ab);
      if (denom <= 0.000001f) {
        simplex = {a};
        return ClosestPointPoint(simplex);
      }

      float t = -Core::Math::dotProduct(a.v, ab) / denom;

      if (t <= 0.0f) {
        simplex = {a};
        return ClosestPointPoint(simplex);
      }

      if (t >= 1.0f) {
        simplex = {b};
        return ClosestPointPoint(simplex);
      }

      Core::Vector3 closest = a.v + ab * t;
      Core::Vector3 closestA = a.a + (b.a - a.a) * t;
      Core::Vector3 closestB = a.b + (b.b - a.b) * t;

      simplex = {a, b};

      return ClosestResult{
          .containsOrigin = false, .closest = closest, .closestA = closestA, .closestB = closestB};
    }
    ClosestResult ClosestPointTriangle(std::vector<SupportPoint>& simplex) {
      SupportPoint A = simplex[0];
      SupportPoint B = simplex[1];
      SupportPoint C = simplex[2];

      Core::Vector3 a = A.v;
      Core::Vector3 b = B.v;
      Core::Vector3 c = C.v;

      Core::Vector3 ab = b - a;
      Core::Vector3 ac = c - a;
      Core::Vector3 ap = a * -1.0f;

      float d1 = Core::Math::dotProduct(ab, ap);
      float d2 = Core::Math::dotProduct(ac, ap);

      if (d1 <= 0.0f && d2 <= 0.0f) {
        simplex = {A};
        return ClosestPointPoint(simplex);
      }

      Core::Vector3 bp = b * -1.0f;
      float d3 = Core::Math::dotProduct(ab, bp);
      float d4 = Core::Math::dotProduct(ac, bp);

      if (d3 >= 0.0f && d4 <= d3) {
        simplex = {B};
        return ClosestPointPoint(simplex);
      }

      float vc = d1 * d4 - d3 * d2;
      if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        simplex = {A, B};
        return ClosestPointLine(simplex);
      }

      Core::Vector3 cp = c * -1.0f;
      float d5 = Core::Math::dotProduct(ab, cp);
      float d6 = Core::Math::dotProduct(ac, cp);

      if (d6 >= 0.0f && d5 <= d6) {
        simplex = {C};
        return ClosestPointPoint(simplex);
      }

      float vb = d5 * d2 - d1 * d6;
      if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        simplex = {A, C};
        return ClosestPointLine(simplex);
      }

      float va = d3 * d6 - d5 * d4;
      if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        simplex = {B, C};
        return ClosestPointLine(simplex);
      }

      float denom = 1.0f / (va + vb + vc);

      float u = va * denom;
      float v = vb * denom;
      float w = vc * denom;

      Core::Vector3 closest = A.v * u + B.v * v + C.v * w;

      Core::Vector3 closestA = A.a * u + B.a * v + C.a * w;

      Core::Vector3 closestB = A.b * u + B.b * v + C.b * w;

      simplex = {A, B, C};

      return ClosestResult{
          .containsOrigin = false, .closest = closest, .closestA = closestA, .closestB = closestB};
    }
    ClosestResult ClosestPointTetrahedron(std::vector<SupportPoint>& simplex) {
      SupportPoint A = simplex[0];
      SupportPoint B = simplex[1];
      SupportPoint C = simplex[2];
      SupportPoint D = simplex[3];

      auto OriginOutsideFace = [&](const SupportPoint& p0, const SupportPoint& p1,
                                   const SupportPoint& p2, const SupportPoint& opposite) -> bool {
        Core::Vector3 ab = p1.v - p0.v;
        Core::Vector3 ac = p2.v - p0.v;

        Core::Vector3 normal = Core::Math::crossProduct(ab, ac);

        // Make normal point away from the opposite tetrahedron point.
        if (Core::Math::dotProduct(normal, opposite.v - p0.v) > 0.0f) {
          normal = normal * -1.0f;
        }

        // Origin is outside if it is in the direction of the outward normal.
        return Core::Math::dotProduct(normal, p0.v * -1.0f) > 0.0f;
      };

      bool outsideABC = OriginOutsideFace(A, B, C, D);
      bool outsideACD = OriginOutsideFace(A, C, D, B);
      bool outsideADB = OriginOutsideFace(A, D, B, C);
      bool outsideBCD = OriginOutsideFace(B, C, D, A);

      if (!outsideABC && !outsideACD && !outsideADB && !outsideBCD) {
        return ClosestResult{.containsOrigin = true};
      }

      float bestDistSq = INFINITY;
      ClosestResult bestResult{};
      std::vector<SupportPoint> bestSimplex{};

      auto TestFace = [&](std::vector<SupportPoint> face) {
        ClosestResult result = ClosestPointTriangle(face);
        float distSq = Core::Math::dotProduct(result.closest, result.closest);

        if (distSq < bestDistSq) {
          bestDistSq = distSq;
          bestResult = result;
          bestSimplex = face;
        }
      };

      if (outsideABC)
        TestFace({A, B, C});
      if (outsideACD)
        TestFace({A, C, D});
      if (outsideADB)
        TestFace({A, D, B});
      if (outsideBCD)
        TestFace({B, C, D});

      simplex = bestSimplex;
      return bestResult;
    }
    ClosestResult ClosestPointSimplex(std::vector<SupportPoint>& simplex) {
      switch (simplex.size()) {
      case 1:
        return ClosestPointPoint(simplex);
      case 2:
        return ClosestPointLine(simplex);
      case 3:
        return ClosestPointTriangle(simplex);
      case 4:
        return ClosestPointTetrahedron(simplex);
      }

      return ClosestResult{};
    }
    GJKDistanceResult GJKDistance(const std::vector<Core::Vector3>& vertsA,
                                  const std::vector<Core::Vector3>& vertsB) {
      const float epsilon = 0.000001f;
      const int maxIterations = 32;

      std::vector<SupportPoint> simplex;

      Core::Vector3 direction{1.0f, 0.0f, 0.0f};

      SupportPoint first = SupportDistance(vertsA, vertsB, direction);
      simplex.push_back(first);

      ClosestResult closest = ClosestPointSimplex(simplex);

      for (int iter = 0; iter < maxIterations; iter++) {
        float distSq = Core::Math::dotProduct(closest.closest, closest.closest);

        if (distSq <= epsilon * epsilon) {
          std::vector<Core::Vector3> epaSimplex;
          for (const SupportPoint& p : simplex) {
            epaSimplex.push_back(p.v);
          }

          return GJKDistanceResult{.overlapped = true,
                                   .distance = 0.0f,
                                   .normal = Core::Vector3{0, 0, 0},
                                   .closestA = closest.closestA,
                                   .closestB = closest.closestB,
                                   .epaSimplex = epaSimplex};
        }

        direction = closest.closest * -1.0f;

        SupportPoint support = SupportDistance(vertsA, vertsB, direction);

        float oldProjection = Core::Math::dotProduct(closest.closest, direction);
        float newProjection = Core::Math::dotProduct(support.v, direction);

        // No meaningful progress, current simplex is the closest feature.
        if (newProjection - oldProjection <= epsilon) {
          float distance = sqrtf(distSq);

          Core::Vector3 normal = Core::Vector3{0, 0, 0};

          if (distance > epsilon) {
            // Normal from A to B.
            normal = (closest.closestB - closest.closestA) * (1.0f / distance);
          }

          return GJKDistanceResult{.overlapped = false,
                                   .distance = distance,
                                   .normal = normal,
                                   .closestA = closest.closestA,
                                   .closestB = closest.closestB};
        }

        simplex.insert(simplex.begin(), support);

        closest = ClosestPointSimplex(simplex);

        if (closest.containsOrigin) {
          std::vector<Core::Vector3> epaSimplex;
          for (const SupportPoint& p : simplex) {
            epaSimplex.push_back(p.v);
          }

          return GJKDistanceResult{.overlapped = true,
                                   .distance = 0.0f,
                                   .normal = Core::Vector3{0, 0, 0},
                                   .closestA = closest.closestA,
                                   .closestB = closest.closestB,
                                   .epaSimplex = epaSimplex};
        }
      }

      float distSq = Core::Math::dotProduct(closest.closest, closest.closest);
      float distance = sqrtf(distSq);

      Core::Vector3 normal{0, 0, 0};
      if (distance > epsilon) {
        normal = (closest.closestB - closest.closestA) * (1.0f / distance);
      }

      return GJKDistanceResult{.overlapped = false,
                               .distance = distance,
                               .normal = normal,
                               .closestA = closest.closestA,
                               .closestB = closest.closestB};
    }

    EPAFace MakeFace(const std::vector<Core::Vector3>& polytope, size_t a, size_t b, size_t c) {
      Core::Vector3 ab = polytope[b] - polytope[a];
      Core::Vector3 ac = polytope[c] - polytope[a];

      Core::Vector3 normal = Core::Math::Vector3Normalize(Core::Math::crossProduct(ab, ac));
      float distance = Core::Math::dotProduct(normal, polytope[a]);

      if (distance < 0.0f) {
        std::swap(b, c);
        normal = normal * -1.0f;
        distance = distance * -1.0f;
      }

      return EPAFace{a, b, c, normal, distance};
    }

    void AddUniqueEdge(std::vector<std::pair<size_t, size_t>>& edges, size_t a, size_t b) {
      auto reverse = std::find(edges.begin(), edges.end(), std::make_pair(b, a));

      if (reverse != edges.end())
        edges.erase(reverse);
      else
        edges.emplace_back(a, b);
    }

    Manifold EPA(const std::vector<Core::Vector3>& simplex,
                 const std::vector<Core::Vector3>& vertsA,
                 const std::vector<Core::Vector3>& vertsB) {
      const float epsilon = 0.00001f;
      const size_t maxIterations = 64;

      std::vector<Core::Vector3> polytope = simplex;

      std::vector<size_t> faces = {0, 1, 2, 0, 3, 1, 0, 2, 3, 1, 3, 2};

      for (size_t iteration = 0; iteration < maxIterations; iteration++) {
        std::vector<EPAFace> faceData;
        float closestDistance = INFINITY;
        size_t closestFaceIndex = 0;

        for (size_t i = 0; i < faces.size(); i += 3) {
          EPAFace face = MakeFace(polytope, faces[i], faces[i + 1], faces[i + 2]);

          faceData.push_back(face);

          if (face.distance < closestDistance) {
            closestDistance = face.distance;
            closestFaceIndex = faceData.size() - 1;
          }
        }

        EPAFace closestFace = faceData[closestFaceIndex];

        Core::Vector3 support = Support(vertsA, vertsB, closestFace.normal);

        float supportDistance = Core::Math::dotProduct(support, closestFace.normal);

        if (supportDistance - closestFace.distance < epsilon) {
          return Manifold{
              .hasCollision = true, .normal = closestFace.normal, .depth = closestFace.distance};
        }

        std::vector<std::pair<size_t, size_t>> uniqueEdges;
        std::vector<size_t> newFaces;

        for (const EPAFace& face : faceData) {
          bool canSeeFace = Core::Math::dotProduct(face.normal, support) > face.distance + epsilon;

          if (canSeeFace) {
            AddUniqueEdge(uniqueEdges, face.a, face.b);
            AddUniqueEdge(uniqueEdges, face.b, face.c);
            AddUniqueEdge(uniqueEdges, face.c, face.a);
          } else {
            newFaces.push_back(face.a);
            newFaces.push_back(face.b);
            newFaces.push_back(face.c);
          }
        }

        size_t newPointIndex = polytope.size();
        polytope.push_back(support);

        for (auto [a, b] : uniqueEdges) {
          newFaces.push_back(a);
          newFaces.push_back(b);
          newFaces.push_back(newPointIndex);
        }

        faces = newFaces;
      }

      return Manifold{};
    }

    Manifold CheckCollisionAtPosition(Core::Vector3 positionToCheck,
                                      const Core::PhysicsBodyState& colBody) {
      Manifold manifold{};
      if (colBody.physicsBodyType == Core::PhysicsBodyType::Dynamic)
        return manifold;

      std::vector<Core::Vector3> m_vertsA = BuildBoxVerts(body, positionToCheck);
      std::vector<Core::Vector3> m_vertsB = BuildBoxVerts(colBody, colBody.position);

      std::vector<Core::Vector3> simplex{};
      Core::Vector3 direction = positionToCheck - colBody.position;
      Core::Vector3 support = Support(m_vertsA, m_vertsB, direction);
      simplex.insert(simplex.begin(), support);
      direction = -1 * support;

      for (int iter = 0; iter < 32; iter++) {
        support = Support(m_vertsA, m_vertsB, direction);

        if (Core::Math::dotProduct(support, direction) <= 0) {
          break;
        }

        simplex.insert(simplex.begin(), support);

        if (ProcessSimplex(simplex, direction)) {
          manifold = EPA(simplex, m_vertsA, m_vertsB);

          break;
        }
      }
      return manifold;
    }
    std::vector<Core::Vector3> BuildBoxVerts(const Core::PhysicsBodyState& targetBody,
                                             Core::Vector3 position) {
      const Core::Vector3 h = targetBody.colliders.at(0).halfExtents;

      return {{position.x + h.x, position.y + h.y, position.z + h.z},
              {position.x + h.x, position.y + h.y, position.z - h.z},
              {position.x + h.x, position.y - h.y, position.z + h.z},
              {position.x + h.x, position.y - h.y, position.z - h.z},

              {position.x - h.x, position.y + h.y, position.z + h.z},
              {position.x - h.x, position.y + h.y, position.z - h.z},
              {position.x - h.x, position.y - h.y, position.z + h.z},
              {position.x - h.x, position.y - h.y, position.z - h.z}};
    }
    GJKDistanceResult GJKDistanceAtPosition(Core::Vector3 positionToCheck,
                                            const Core::PhysicsBodyState& colBody) {
      std::vector<Core::Vector3> vertsA = BuildBoxVerts(body, positionToCheck);

      std::vector<Core::Vector3> vertsB = BuildBoxVerts(colBody, colBody.position);

      GJKDistanceResult gjk = GJKDistance(vertsA, vertsB);

      if (gjk.overlapped) {
        gjk.distance = 0.0f;

        if (gjk.epaSimplex.size() >= 4) {
          Manifold m = EPA(gjk.epaSimplex, vertsA, vertsB);
          gjk.normal = m.normal;
        }

        return gjk;
      }

      // Flip for your sweep logic.
      gjk.normal = gjk.normal * -1.0f;

      return gjk;
    }
    SweepHit ConvexCastAgainstBody(Core::Vector3 startPosition, Core::Vector3 displacement,
                                   const Core::PhysicsBodyState& colBody) {
      SweepHit result{};

      float t = 0.0f;
      constexpr int maxIterations = 32;
      constexpr float skin = 0.001f;
      constexpr float epsilon = 0.00001f;

      for (int i = 0; i < maxIterations; i++) {
        Core::Vector3 testPosition = startPosition + displacement * t;

        GJKDistanceResult gjk = GJKDistanceAtPosition(testPosition, colBody);

        if (gjk.overlapped) {
          // If this happens at t = 0, recovery failed or numerical slop exists.
          // Do not turn this into a slide hit.
          if (t <= epsilon)
            return result;

          result.hasHit = true;
          result.t = t;
          result.normal = gjk.normal;
          return result;
        }

        if (gjk.distance <= skin) {
          float movingIntoSurface = Core::Math::dotProduct(displacement, gjk.normal);

          if (movingIntoSurface < 0.0f) {
            result.hasHit = true;
            result.t = t;
            result.normal = gjk.normal;
            return result;
          }

          return result;
        }

        float closingSpeed = -Core::Math::dotProduct(displacement, gjk.normal);

        if (closingSpeed <= epsilon)
          return result;

        float advance = (gjk.distance - skin) / closingSpeed;
        t += advance;

        if (t > 1.0f)
          return result;
      }

      return result;
    }
    SweepHit SweepAgainstBodies(Core::Vector3 startPosition, Core::Vector3 displacement,
                                std::vector<Core::PhysicsBodyState>& bodies) {
      SweepHit closestHit{};
      closestHit.t = 1.0f;

      for (size_t i = 0; i < bodies.size(); i++) {
        Core::PhysicsBodyState& colBody = bodies[i];

        if (colBody.physicsBodyType == Core::PhysicsBodyType::Dynamic) {
          continue;
        }

        SweepHit hit = ConvexCastAgainstBody(startPosition, displacement, colBody);

        if (hit.hasHit && hit.t < closestHit.t) {
          closestHit = hit;
        }
      }

      return closestHit;
    }
    bool RecoverFromPenetration(Core::Vector3& position,
                                const std::vector<Core::PhysicsBodyState>& bodies) {
      constexpr int maxPasses = 8;
      constexpr float skin = 0.001f;
      constexpr float epsilon = 0.00001f;

      for (int pass = 0; pass < maxPasses; pass++) {
        Manifold deepest{};
        float deepestDepth = 0.0f;

        for (const auto& colBody : bodies) {
          if (colBody.physicsBodyType == Core::PhysicsBodyType::Dynamic)
            continue;

          Manifold m = CheckCollisionAtPosition(position, colBody);

          if (!m.hasCollision)
            continue;

          if (m.depth > deepestDepth) {
            deepest = m;
            deepestDepth = m.depth;
          }
        }

        if (!deepest.hasCollision)
          return true;

        Core::Vector3 normal = Core::Math::Vector3Normalize(deepest.normal);

        if (Core::Math::Length(normal) <= epsilon)
          return false;

        position = position + normal * (deepest.depth + skin);
      }

      return !IsOverlappingAny(position, bodies);
    }
    bool IsOverlappingAny(Core::Vector3 position,
                          const std::vector<Core::PhysicsBodyState>& bodies) {
      for (const auto& colBody : bodies) {
        if (colBody.physicsBodyType == Core::PhysicsBodyType::Dynamic)
          continue;

        Manifold m = CheckCollisionAtPosition(position, colBody);

        if (m.hasCollision)
          return true;
      }

      return false;
    }
    void KinematicMove(Core::Vector3 displacement, std::vector<Core::PhysicsBodyState> bodies) {
      constexpr int maxSlideIterations = 5;
      constexpr float skin = 0.001f;
      constexpr float minMoveDistance = 0.0001f;

      Core::Vector3 position = body.position;

      // Only recover before movement.
      if (IsOverlappingAny(position, bodies)) {
        if (!RecoverFromPenetration(position, bodies)) {
          body.position = position;
          return;
        }
      }

      Core::Vector3 remainingMove = displacement;
      std::vector<Core::Vector3> planes;

      for (int i = 0; i < maxSlideIterations; i++) {
        float moveLength = Core::Math::Length(remainingMove);

        if (moveLength <= minMoveDistance)
          break;

        SweepHit hit = SweepAgainstBodies(position, remainingMove, bodies);

        if (!hit.hasHit) {
          Core::Vector3 targetPosition = position + remainingMove;

          if (!IsOverlappingAny(targetPosition, bodies))
            position = targetPosition;

          break;
        }

        Core::Vector3 normal = Core::Math::Vector3Normalize(hit.normal);

        if (Core::Math::Length(normal) <= 0.0001f)
          break;

        float safeT = hit.t - (skin / moveLength);

        if (safeT < 0.0f)
          safeT = 0.0f;

        Core::Vector3 newPosition = position + remainingMove * safeT;

        if (IsOverlappingAny(newPosition, bodies))
          break;

        position = newPosition;

        Core::Vector3 leftoverMove = remainingMove * (1.0f - safeT);

        planes.push_back(normal);

        Core::Vector3 slideMove = leftoverMove;

        for (Core::Vector3 planeNormal : planes) {
          float intoPlane = Core::Math::dotProduct(slideMove, planeNormal);

          if (intoPlane < 0.0f)
            slideMove = slideMove - planeNormal * intoPlane;
        }

        remainingMove = slideMove;
      }

      body.position = position;
    }
  };
} // namespace Anjean::Runtime
