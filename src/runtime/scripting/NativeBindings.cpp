#include "NativeBindings.h"

#include "../Runtime.h"
#include "../RuntimeTypes.h"
#include "../objects/GameObject.h"
#include "../../Core/Core.h"
#include "../objects/Camera.h"

namespace
{
    Anjean::Runtime::Runtime* g_runtime = nullptr;

    constexpr int ANJEAN_OK = 0;
    constexpr int ANJEAN_ERR_NO_RUNTIME = -1;
    constexpr int ANJEAN_ERR_NULL_ARGUMENT = -2;
    constexpr int ANJEAN_ERR_GAME_OBJECT_NOT_FOUND = -3;
    constexpr int ANJEAN_ERR_UNKNOWN = -999;
    constexpr int ANJEAN_ERR_WRONG_OBJECT_TYPE = -5;
    constexpr int ANJEAN_ERR_MESH_NOT_FOUND = -6;
    constexpr int ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND = -7;


    Anjean::Runtime::Camera& getCameraById(std::uint32_t cameraId)
    {
        auto& object = g_runtime->getGameObjectById(cameraId);

        if (object.getGameObjectType() != Anjean::Runtime::ANJEAN_GAMEOBJECT_CAMERA)
        {
            throw std::bad_cast();
        }

        return static_cast<Anjean::Runtime::Camera&>(object);
    }
}

namespace Anjean::Runtime
{
    void BindNativeRuntime(Runtime* runtime)
    {
        g_runtime = runtime;
    }
}

extern "C"
{
  int Anjean_Runtime_CreateGameObject(std::uint32_t* outGameObjectId)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outGameObjectId)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& object = g_runtime->createGameObject();
        *outGameObjectId = object.id;

        return ANJEAN_OK;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

  int Anjean_Runtime_CreateCamera(std::uint32_t* outCameraId)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outCameraId)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& camera = g_runtime->createCamera();
        *outCameraId = camera.id;

        return ANJEAN_OK;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_Camera_SetFieldOfView(
    std::uint32_t cameraId,
    float fieldOfView
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& camera = getCameraById(cameraId);
        camera.fieldOfView = fieldOfView;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (const std::bad_cast&)
    {
        return ANJEAN_ERR_WRONG_OBJECT_TYPE;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_Camera_SetNearClippingPlane(
    std::uint32_t cameraId,
    float nearClippingPlane
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& camera = getCameraById(cameraId);
        camera.nearClippingPlane = nearClippingPlane;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (const std::bad_cast&)
    {
        return ANJEAN_ERR_WRONG_OBJECT_TYPE;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_Camera_SetFarClippingPlane(
    std::uint32_t cameraId,
    float farClippingPlane
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& camera = getCameraById(cameraId);
        camera.farClippingPlane = farClippingPlane;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (const std::bad_cast&)
    {
        return ANJEAN_ERR_WRONG_OBJECT_TYPE;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_Camera_SetAspectRatio(
    std::uint32_t cameraId,
    float aspectRatio
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& camera = getCameraById(cameraId);
        camera.aspectRatio = aspectRatio;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (const std::bad_cast&)
    {
        return ANJEAN_ERR_WRONG_OBJECT_TYPE;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_Runtime_SetCurrentCamera(std::uint32_t cameraId)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& camera = getCameraById(cameraId);

        g_runtime->setCurrentCamera(camera.id);

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (const std::bad_cast&)
    {
        return ANJEAN_ERR_WRONG_OBJECT_TYPE;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

  int Anjean_Runtime_GetCurrentCamera(std::uint32_t* outCameraId)
  {
      if (!g_runtime)
      {
          return ANJEAN_ERR_NO_RUNTIME;
      }

      if (!outCameraId)
      {
          return ANJEAN_ERR_NULL_ARGUMENT;
      }

      try
      {
          *outCameraId = g_runtime->getCurrentCamera().id;
          return ANJEAN_OK;
      }
      catch (const std::runtime_error&)
      {
          return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
      }
      catch (...)
      {
          return ANJEAN_ERR_UNKNOWN;
      }
  }


int Anjean_GameObject_GetPosition(
    std::uint32_t gameObjectId,
    Anjean::Core::Vector3* outPosition
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outPosition)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        outPosition->x = object.transform.position.x;
        outPosition->y = object.transform.position.y;
        outPosition->z = object.transform.position.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_GameObject_SetPosition(
    std::uint32_t gameObjectId,
    Anjean::Core::Vector3 position
){
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        object.transform.position.x = position.x;
        object.transform.position.y = position.y;
        object.transform.position.z = position.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_GameObject_GetRotation(
    std::uint32_t gameObjectId,
    Anjean::Core::Vector3* outRotation
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outRotation)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        outRotation->x = object.transform.rotation.x;
        outRotation->y = object.transform.rotation.y;
        outRotation->z = object.transform.rotation.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_GameObject_SetRotation(
    std::uint32_t gameObjectId,
    Anjean::Core::Vector3 rotation
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        object.transform.rotation.x = rotation.x;
        object.transform.rotation.y = rotation.y;
        object.transform.rotation.z = rotation.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_GameObject_SetMesh(
    std::uint32_t gameObjectId,
    std::uint32_t meshId
)
{
    if (!g_runtime)
    {
        return -1;
    }

    if (meshId == 0)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        std::vector<Anjean::Runtime::Mesh> meshes = g_runtime->getAllMeshes();

        for (const Anjean::Runtime::Mesh& mesh : meshes)
        {
            if (mesh.id == meshId)
            {
                object.mesh = mesh;
                return ANJEAN_OK;
            }
        }

        return -4; // mesh id not found
    }
    catch (...)
    {
        return -999;
    }
  }

  int Anjean_GameObject_SetTexture(
    std::uint32_t gameObjectId,
    const char* filename,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t channels
)
{
    if (!g_runtime)
    {
        return -1;
    }

    if (!filename)
    {
        return -2;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        Anjean::Runtime::Texture texture{};
        texture.filename = filename;
        texture.width = width;
        texture.height = height;
        texture.channels = channels;

        object.texture = texture;

        return 0;
    }
    catch (...)
    {
        return -999;
    }
  }

  int32_t Anjean_Input_IsKeyDown(int32_t keyCode)
  {
      if (!g_runtime)
      {
          return 0;
      }

      auto key = static_cast<Anjean::Runtime::Key>(keyCode);

      if (key <= Anjean::Runtime::Key::Unknown ||
          key >= Anjean::Runtime::Key::Count)
      {
          return 0;
      }

      return g_runtime->inputManager.isKeyDown(key) ? 1 : 0;
  }

  int Anjean_PhysicsBody_GetType(
    std::uint32_t physicsBodyId,
    std::uint32_t* outPhysicsBodyType
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outPhysicsBodyType)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        *outPhysicsBodyType = static_cast<std::uint32_t>(
            body.getPhysicsBodyType()
        );

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_SetVelocity(
    std::uint32_t physicsBodyId,
    Anjean::Core::Vector3 velocity
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        body.velocity.x = velocity.x;
        body.velocity.y = velocity.y;
        body.velocity.z = velocity.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_GetVelocity(
    std::uint32_t physicsBodyId,
    Anjean::Core::Vector3* outVelocity
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outVelocity)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        outVelocity->x = body.velocity.x;
        outVelocity->y = body.velocity.y;
        outVelocity->z = body.velocity.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_SetForce(
    std::uint32_t physicsBodyId,
    Anjean::Core::Vector3 force
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        body.force.x = force.x;
        body.force.y = force.y;
        body.force.z = force.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_GetForce(
    std::uint32_t physicsBodyId,
    Anjean::Core::Vector3* outForce
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outForce)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        outForce->x = body.force.x;
        outForce->y = body.force.y;
        outForce->z = body.force.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_SetMass(
    std::uint32_t physicsBodyId,
    float mass
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        body.mass = mass;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_GetMass(
    std::uint32_t physicsBodyId,
    float* outMass
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outMass)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        *outMass = body.mass;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}
int Anjean_PhysicsBody_AddRectangularPrismCollider(
    std::uint32_t physicsBodyId,
    Anjean::Core::Vector3 localCenter,
    Anjean::Core::Vector3 halfExtents,
    std::uint32_t* outColliderId
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outColliderId)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        Anjean::Runtime::Collider collider{};

        collider.id = body.nextColliderId++;
        collider.type = Anjean::Core::ColliderType::RectangularPrism;

        collider.localCenter = Anjean::Core::Vector3{
            localCenter.x,
            localCenter.y,
            localCenter.z
        };

        collider.halfExtents = Anjean::Core::Vector3{
            halfExtents.x,
            halfExtents.y,
            halfExtents.z
        };

        body.colliders.push_back(collider);

        *outColliderId = collider.id;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_Create(
    std::uint32_t physicsBodyType,
    std::uint32_t* outPhysicsBodyId
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outPhysicsBodyId)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto type = static_cast<Anjean::Core::PhysicsBodyType>(physicsBodyType);

        auto& body = g_runtime->createPhysicsBody(type);

        *outPhysicsBodyId = body.id;

        return ANJEAN_OK;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_GameObject_SetPhysicsBody(
    std::uint32_t gameObjectId,
    std::uint32_t physicsBodyId
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        g_runtime->setGameObjectPhysicsBody(gameObjectId, physicsBodyId);

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_GameObject_GetPhysicsBody(
    std::uint32_t gameObjectId,
    std::uint32_t* outPhysicsBodyId
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outPhysicsBodyId)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        *outPhysicsBodyId = g_runtime->getGameObjectPhysicsBody(gameObjectId);

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_GetColliderCount(
    std::uint32_t physicsBodyId,
    int* outCount
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outCount)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        *outCount = static_cast<int>(body.colliders.size());

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_GetColliderAt(
    std::uint32_t physicsBodyId,
    int index,
    std::uint32_t* outColliderType,
    std::uint32_t* outColliderId
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outColliderType || !outColliderId)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        if (index < 0 || index >= static_cast<int>(body.colliders.size()))
        {
            return ANJEAN_ERR_UNKNOWN;
        }

        const auto& collider = body.colliders[static_cast<std::size_t>(index)];

        *outColliderType = static_cast<std::uint32_t>(collider.type);
        *outColliderId = collider.id;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_AddSphereCollider(
    std::uint32_t physicsBodyId,
    Anjean::Core::Vector3 localCenter,
    float radius,
    std::uint32_t* outColliderId
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outColliderId)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        Anjean::Runtime::Collider collider{};

        collider.id = body.nextColliderId++;
        collider.type = Anjean::Core::ColliderType::Sphere;
        collider.localCenter = localCenter;
        collider.radius = radius;

        body.colliders.push_back(collider);

        *outColliderId = collider.id;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_PhysicsBody_RemoveCollider(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (auto it = body.colliders.begin(); it != body.colliders.end(); ++it)
        {
            if (it->id == colliderId)
            {
                body.colliders.erase(it);
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_SphereCollider_GetLocalCenter(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3* outLocalCenter
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outLocalCenter)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (const auto& collider : body.colliders)
        {
            if (collider.id == colliderId)
            {
                if (collider.type != Anjean::Core::ColliderType::Sphere)
                {
                    return ANJEAN_ERR_WRONG_OBJECT_TYPE;
                }

                *outLocalCenter = collider.localCenter;
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_SphereCollider_SetLocalCenter(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3 localCenter
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (auto& collider : body.colliders)
        {
            if (collider.id == colliderId)
            {
                if (collider.type != Anjean::Core::ColliderType::Sphere)
                {
                    return ANJEAN_ERR_WRONG_OBJECT_TYPE;
                }

                collider.localCenter = localCenter;
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_SphereCollider_GetRadius(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    float* outRadius
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outRadius)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (const auto& collider : body.colliders)
        {
            if (collider.id == colliderId)
            {
                if (collider.type != Anjean::Core::ColliderType::Sphere)
                {
                    return ANJEAN_ERR_WRONG_OBJECT_TYPE;
                }

                *outRadius = collider.radius;
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_SphereCollider_SetRadius(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    float radius
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (auto& collider : body.colliders)
        {
            if (collider.id == colliderId)
            {
                if (collider.type != Anjean::Core::ColliderType::Sphere)
                {
                    return ANJEAN_ERR_WRONG_OBJECT_TYPE;
                }

                collider.radius = radius;
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_RectangularPrismCollider_GetLocalCenter(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3* outLocalCenter
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outLocalCenter)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (const auto& collider : body.colliders)
        {
            if (collider.id == colliderId)
            {
                if (collider.type != Anjean::Core::ColliderType::RectangularPrism)
                {
                    return ANJEAN_ERR_WRONG_OBJECT_TYPE;
                }

                *outLocalCenter = collider.localCenter;
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_RectangularPrismCollider_SetLocalCenter(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3 localCenter
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (auto& collider : body.colliders)
        {
            if (collider.id == colliderId)
            {
                if (collider.type != Anjean::Core::ColliderType::RectangularPrism)
                {
                    return ANJEAN_ERR_WRONG_OBJECT_TYPE;
                }

                collider.localCenter = localCenter;
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_RectangularPrismCollider_GetHalfExtents(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3* outHalfExtents
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outHalfExtents)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (const auto& collider : body.colliders)
        {
            if (collider.id == colliderId)
            {
                if (collider.type != Anjean::Core::ColliderType::RectangularPrism)
                {
                    return ANJEAN_ERR_WRONG_OBJECT_TYPE;
                }

                *outHalfExtents = collider.halfExtents;
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_RectangularPrismCollider_SetHalfExtents(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3 halfExtents
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& body = g_runtime->getPhysicsBodyById(physicsBodyId);

        for (auto& collider : body.colliders)
        {
            if (collider.id == colliderId)
            {
                if (collider.type != Anjean::Core::ColliderType::RectangularPrism)
                {
                    return ANJEAN_ERR_WRONG_OBJECT_TYPE;
                }

                collider.halfExtents = halfExtents;
                return ANJEAN_OK;
            }
        }

        return ANJEAN_ERR_UNKNOWN;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_PHYSICS_BODY_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

}