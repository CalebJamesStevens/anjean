#pragma once

#include <vector>
#include "../Core/Core.h"

namespace Anjean::Physics
{
    class Physics
    {
    private:
        Core::Vector3 m_gravity = Core::Vector3{0, -9.81f, 0};

    public:
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
        std::vector<Core::PhysicsBodyResult> Step(
            float dt,
            std::vector<Core::PhysicsBodyState> bodies
        )
        {
            std::vector<Core::PhysicsBodyResult> results;
            results.reserve(bodies.size());

            for (Core::PhysicsBodyState& body : bodies)
            {
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

                        bool collision = false;
                        for (Core::PhysicsBodyState& colBody : bodies) {
                          if (colBody.id == body.id) {
                            continue;
                          }

                    
                          float maxDistance = -INFINITY;
                          
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

                          while (true) {
                            support = Support(m_vertsA, m_vertsB, direction);

                            if (dotProduct(support, direction) <= 0) {
                              break;
                            }

                            simplex.insert(simplex.begin(), support);
                            
                            if (ProcessSimplex(simplex, direction)) {
                              collision = true;
                              break;
                            }
                          }

                        }
                        
                        if (collision) {
                          body.velocity = {0};
                        }

                        body.position += body.velocity * dt;

                        results.push_back(Core::PhysicsBodyResult{
                            .id = body.id,
                            .position = body.position,
                            .velocity = body.velocity
                        });

                        break;
                    }
                }
            }

            return results;
        }
    };
}