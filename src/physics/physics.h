#pragma once

#include <vector>
#include "../Core/Core.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Anjean::Physics
{
    class Physics
    {
    private:
        Core::Vector3 m_gravity = Core::Vector3{0, -9.81f, 0};

    public:
            struct Manifold
    {
        bool hasCollision = false;
        Core::Vector3 normal{};
        float depth = 0.0f;
    };

    struct EPAFace
    {
        size_t a, b, c;
        Core::Vector3 normal;
        float distance;
    };

    float Length(const Core::Vector3& v)
    {
        return std::sqrt(dotProduct(v, v));
    }

    Core::Vector3 Normalize(const Core::Vector3& v)
    {
        float len = Length(v);
        if (len <= 0.000001f) return Core::Vector3{0, 0, 0};
        return v / len;
    }

    EPAFace MakeFace(
        const std::vector<Core::Vector3>& polytope,
        size_t a,
        size_t b,
        size_t c
    )
    {
        Core::Vector3 ab = polytope[b] - polytope[a];
        Core::Vector3 ac = polytope[c] - polytope[a];

        Core::Vector3 normal = Normalize(crossProduct(ab, ac));
        float distance = dotProduct(normal, polytope[a]);

        if (distance < 0.0f)
        {
            std::swap(b, c);
            normal = normal * -1.0f;
            distance = distance * -1.0f;
        }

        return EPAFace{a, b, c, normal, distance};
    }

    void AddUniqueEdge(
        std::vector<std::pair<size_t, size_t>>& edges,
        size_t a,
        size_t b
    )
    {
        auto reverse = std::find(
            edges.begin(),
            edges.end(),
            std::make_pair(b, a)
        );

        if (reverse != edges.end())
            edges.erase(reverse);
        else
            edges.emplace_back(a, b);
    }

    Manifold EPA(
        const std::vector<Core::Vector3>& simplex,
        const std::vector<Core::Vector3>& vertsA,
        const std::vector<Core::Vector3>& vertsB
    )
    {
        const float epsilon = 0.00001f;
        const size_t maxIterations = 64;

        std::vector<Core::Vector3> polytope = simplex;

        std::vector<size_t> faces = {
            0, 1, 2,
            0, 3, 1,
            0, 2, 3,
            1, 3, 2
        };

        for (size_t iteration = 0; iteration < maxIterations; iteration++)
        {
            std::vector<EPAFace> faceData;
            float closestDistance = INFINITY;
            size_t closestFaceIndex = 0;

            for (size_t i = 0; i < faces.size(); i += 3)
            {
                EPAFace face = MakeFace(
                    polytope,
                    faces[i],
                    faces[i + 1],
                    faces[i + 2]
                );

                faceData.push_back(face);

                if (face.distance < closestDistance)
                {
                    closestDistance = face.distance;
                    closestFaceIndex = faceData.size() - 1;
                }
            }

            EPAFace closestFace = faceData[closestFaceIndex];

            Core::Vector3 support =
                Support(vertsA, vertsB, closestFace.normal);

            float supportDistance =
                dotProduct(support, closestFace.normal);

            if (supportDistance - closestFace.distance < epsilon)
            {
                return Manifold{
                    .hasCollision = true,
                    .normal = closestFace.normal,
                    .depth = closestFace.distance
                };
            }

            std::vector<std::pair<size_t, size_t>> uniqueEdges;
            std::vector<size_t> newFaces;

            for (const EPAFace& face : faceData)
            {
                bool canSeeFace =
                    dotProduct(face.normal, support) > face.distance + epsilon;

                if (canSeeFace)
                {
                    AddUniqueEdge(uniqueEdges, face.a, face.b);
                    AddUniqueEdge(uniqueEdges, face.b, face.c);
                    AddUniqueEdge(uniqueEdges, face.c, face.a);
                }
                else
                {
                    newFaces.push_back(face.a);
                    newFaces.push_back(face.b);
                    newFaces.push_back(face.c);
                }
            }

            size_t newPointIndex = polytope.size();
            polytope.push_back(support);

            for (auto [a, b] : uniqueEdges)
            {
                newFaces.push_back(a);
                newFaces.push_back(b);
                newFaces.push_back(newPointIndex);
            }

            faces = newFaces;
        }

        return Manifold{};
    }
        float dotProduct(const Core::Vector3& a, const Core::Vector3& b) {
          return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
        }
        bool SameDirection(const Core::Vector3& direction, const Core::Vector3& ao)
        {
          return dotProduct(direction, ao) > 0;
        }
        Core::Vector3 crossProduct(const Core::Vector3& a, const Core::Vector3& b) {
            return Core::Vector3{
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            };
        }
        bool Line(std::vector<Core::Vector3>& points, Core::Vector3& direction)
        {
          Core::Vector3 a = points[0];
          Core::Vector3 b = points[1];

          Core::Vector3 ab = b - a;
          Core::Vector3 ao = Core::Vector3{0,0,0} - a;
        
          if (SameDirection(ab, ao)) {
            direction = crossProduct(crossProduct(ab, ao), ab);
          }

          else {
            points = { a };
            direction = ao;
          }

          return false;
        }
        bool Triangle(std::vector<Core::Vector3>& points, Core::Vector3& direction)
        {
          Core::Vector3 a = points[0];
          Core::Vector3 b = points[1];
          Core::Vector3 c = points[2];

          Core::Vector3 ab = b - a;
          Core::Vector3 ac = c - a;
          Core::Vector3 ao = Core::Vector3{0,0,0} - a;
        
          Core::Vector3 abc = crossProduct(ab, ac);
        
          if (SameDirection(crossProduct(abc, ac), ao)) {
            if (SameDirection(ac, ao)) {
              points = { a, c };
              direction = crossProduct(crossProduct(ac, ao), ac);
            }

            else {
              return Line(points = { a, b }, direction);
            }
          }
        
          else {
            if (SameDirection(crossProduct(ab, abc), ao)) {
              return Line(points = { a, b }, direction);
            }

            else {
              if (SameDirection(abc, ao)) {
                direction = abc;
              }

              else {
                points = { a, c, b };
                direction = Core::Vector3{0,0,0}-abc;
              }
            }
          }

          return false;
        }
        bool Tetrahedron(std::vector<Core::Vector3>& points, Core::Vector3& direction)
        {
          Core::Vector3 a = points[0];
          Core::Vector3 b = points[1];
          Core::Vector3 c = points[2];
          Core::Vector3 d = points[3];

          Core::Vector3 ab = b - a;
          Core::Vector3 ac = c - a;
          Core::Vector3 ad = d - a;
          Core::Vector3 ao = Core::Vector3{0,0,0} - a;
        
          Core::Vector3 abc = crossProduct(ab, ac);
          Core::Vector3 acd = crossProduct(ac, ad);
          Core::Vector3 adb = crossProduct(ad, ab);
        
          if (SameDirection(abc, ao)) {
            return Triangle(points = { a, b, c }, direction);
          }
            
          if (SameDirection(acd, ao)) {
            return Triangle(points = { a, c, d }, direction);
          }
        
          if (SameDirection(adb, ao)) {
            return Triangle(points = { a, d, b }, direction);
          }
        
          return true;
        }
        bool ProcessSimplex (std::vector<Core::Vector3>& points, Core::Vector3& direction) {
          switch (points.size()) {
            case 2: return Line(points, direction);
            case 3: return Triangle(points, direction);
            case 4: return Tetrahedron(points, direction);
          }
          return false;
        }
        Core::Vector3 Support (std::vector<Core::Vector3> m_vertsA, std::vector<Core::Vector3> m_vertsB, Core::Vector3 direction) {
          float maxDistanceA = -INFINITY;
          float maxDistanceB = -INFINITY;
          Core::Vector3  maxPointA;
          Core::Vector3  maxPointB;
          for (Core::Vector3 vertex : m_vertsA) {
            float distance = dotProduct(vertex, direction);
            if (distance > maxDistanceA) {
              maxDistanceA = distance;
              maxPointA = vertex;
            }
          }
          
          for (Core::Vector3 vertex : m_vertsB) {
            float distance = dotProduct(vertex, direction*-1);
            if (distance > maxDistanceB) {
              maxDistanceB = distance;
              maxPointB = vertex;
            }
          }

          return maxPointA - maxPointB;
        }
        void ResolveCollision(
            Core::PhysicsBodyState& A,
            Core::PhysicsBodyState& B,
            const Manifold& manifold,
            float dt
        )
        {
            Core::Vector3 normal = manifold.normal;

            Core::Vector3 relativeVelocity = B.velocity - A.velocity;

            float velocityAlongNormal = dotProduct(relativeVelocity, normal);

            if (velocityAlongNormal > 0.0f)
                return;

            float invMassA =
                A.physicsBodyType == Core::PhysicsBodyType::Dynamic
                    ? 1.0f / A.mass
                    : 0.0f;

            float invMassB =
                B.physicsBodyType == Core::PhysicsBodyType::Dynamic
                    ? 1.0f / B.mass
                    : 0.0f;

            if (invMassA + invMassB == 0.0f)
                return;

            float restitution = 0.9f;
            if (std::abs(velocityAlongNormal) < 0.5f)
            {
                restitution = 0.0f;
            }

            float j = -(1.0f + restitution) * velocityAlongNormal;
            j /= invMassA + invMassB;

            Core::Vector3 impulse = j * normal;

            A.velocity -= invMassA * impulse;
            B.velocity += invMassB * impulse;

            float percent = .8f;
            Core::Vector3 correction =
                normal * (manifold.depth * percent / (invMassA + invMassB));

            A.position -= invMassA * correction;
            B.position += invMassB * correction;

            Core::Vector3 tempPos = A.position + invMassA * correction;

        }
        std::vector<Core::PhysicsBodyResult> Step(
            float dt,
            std::vector<Core::PhysicsBodyState> bodies
        )
        {
            std::vector<Core::PhysicsBodyResult> results;
            results.reserve(bodies.size());

            for (size_t i = 0; i < bodies.size(); i++)
            {
                Core::PhysicsBodyState& body = bodies[i];

                switch (body.physicsBodyType)
                {
                    case Core::PhysicsBodyType::Static:
                    {
                        // Static bodies are not moved by physics.
                        results.push_back(Core::PhysicsBodyResult{
                            .id = body.id,
                            .position = body.position,
                            .velocity = Core::Vector3{0, 0, 0}
                        });

                        break;
                    }

                    case Core::PhysicsBodyType::Kinematic:
                    {
                        // Kinematic bodies are moved intentionally, not by forces.
                        body.position += body.velocity * dt;

                        results.push_back(Core::PhysicsBodyResult{
                            .id = body.id,
                            .position = body.position,
                            .velocity = body.velocity
                        });

                        break;
                    }

                    case Core::PhysicsBodyType::Dynamic:
                    {
                        // Dynamic bodies respond to gravity, force, and mass.
                        if (body.mass <= 0.0f)
                        {
                            break;
                        }

                        body.force += body.mass * m_gravity;

                        body.velocity += (body.force / body.mass) * dt;
                        body.position += body.velocity * dt;

                        // body.force = Core::Vector3{0, 0, 0};

                        std::vector<Core::PhysicsBodyState> collidedBodies;
                        bool collision = false;
                        for (size_t j = i + 1; j < bodies.size(); j++)
                        {
                          Core::PhysicsBodyState& colBody = bodies[j];

                    
                          
                          std::vector<Core::Vector3> m_vertsA{
                            {body.colliders.at(0).halfExtents.x + body.position.x , body.colliders.at(0).halfExtents.y + body.position.y , body.colliders.at(0).halfExtents.z + body.position.z },
                            {body.colliders.at(0).halfExtents.x + body.position.x , body.colliders.at(0).halfExtents.y + body.position.y , -body.colliders.at(0).halfExtents.z + body.position.z },
                            {body.colliders.at(0).halfExtents.x + body.position.x , -body.colliders.at(0).halfExtents.y + body.position.y , body.colliders.at(0).halfExtents.z + body.position.z },
                            {body.colliders.at(0).halfExtents.x + body.position.x , -body.colliders.at(0).halfExtents.y + body.position.y , -body.colliders.at(0).halfExtents.z + body.position.z },
                            {-body.colliders.at(0).halfExtents.x + body.position.x , body.colliders.at(0).halfExtents.y + body.position.y , body.colliders.at(0).halfExtents.z + body.position.z },
                            {-body.colliders.at(0).halfExtents.x + body.position.x , body.colliders.at(0).halfExtents.y + body.position.y , -body.colliders.at(0).halfExtents.z + body.position.z },
                            {-body.colliders.at(0).halfExtents.x + body.position.x , -body.colliders.at(0).halfExtents.y + body.position.y , body.colliders.at(0).halfExtents.z + body.position.z },
                            {-body.colliders.at(0).halfExtents.x + body.position.x , -body.colliders.at(0).halfExtents.y + body.position.y , -body.colliders.at(0).halfExtents.z + body.position.z }
                          };
                          
                          std::vector<Core::Vector3> m_vertsB{
                            {colBody.colliders.at(0).halfExtents.x + colBody.position.x , colBody.colliders.at(0).halfExtents.y + colBody.position.y , colBody.colliders.at(0).halfExtents.z + colBody.position.z },
                            {colBody.colliders.at(0).halfExtents.x + colBody.position.x , colBody.colliders.at(0).halfExtents.y + colBody.position.y , -colBody.colliders.at(0).halfExtents.z + colBody.position.z },
                            {colBody.colliders.at(0).halfExtents.x + colBody.position.x , -colBody.colliders.at(0).halfExtents.y + colBody.position.y , colBody.colliders.at(0).halfExtents.z + colBody.position.z },
                            {colBody.colliders.at(0).halfExtents.x + colBody.position.x , -colBody.colliders.at(0).halfExtents.y + colBody.position.y , -colBody.colliders.at(0).halfExtents.z + colBody.position.z },
                            {-colBody.colliders.at(0).halfExtents.x + colBody.position.x , colBody.colliders.at(0).halfExtents.y + colBody.position.y , colBody.colliders.at(0).halfExtents.z + colBody.position.z },
                            {-colBody.colliders.at(0).halfExtents.x + colBody.position.x , colBody.colliders.at(0).halfExtents.y + colBody.position.y , -colBody.colliders.at(0).halfExtents.z + colBody.position.z },
                            {-colBody.colliders.at(0).halfExtents.x + colBody.position.x , -colBody.colliders.at(0).halfExtents.y + colBody.position.y , colBody.colliders.at(0).halfExtents.z + colBody.position.z },
                            {-colBody.colliders.at(0).halfExtents.x + colBody.position.x , -colBody.colliders.at(0).halfExtents.y + colBody.position.y , -colBody.colliders.at(0).halfExtents.z + colBody.position.z }
                          };

                          std::vector<Core::Vector3> simplex{};
                          Core::Vector3 direction = {1,0,0};
                          Core::Vector3 support = Support(m_vertsA, m_vertsB, direction);
                          simplex.insert(simplex.begin(), support);
                          direction = -1*support;

                          for (int iter = 0; iter < 32; iter++){
                            support = Support(m_vertsA, m_vertsB, direction);

                            if (dotProduct(support, direction) <= 0) {
                              break;
                            }

                            simplex.insert(simplex.begin(), support);

                            if (ProcessSimplex(simplex, direction))
                            {
                                Manifold manifold = EPA(simplex, m_vertsA, m_vertsB);

                                if (manifold.hasCollision)
                                {
                                  ResolveCollision(body, colBody, manifold, dt);
                                }

                                break;
                            }
                          }

                        }


                        // body.position += body.velocity * dt;

                        // results.push_back(Core::PhysicsBodyResult{
                        //     .id = body.id,
                        //     .position = body.position,
                        //     .velocity = body.velocity
                        // });

                        break;
                    }
                }
            }

            for (Core::PhysicsBodyState& body : bodies)
            {
                results.push_back(Core::PhysicsBodyResult{
                    .id = body.id,
                    .position = body.position,
                    .velocity = body.velocity
                });
            }

            return results;
        }
    };
}