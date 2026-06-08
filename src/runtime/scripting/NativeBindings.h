#pragma once

#include <cstdint>
#include "../../Core/Core.h"
#include "../RuntimeTypes.h"

namespace Anjean::Runtime
{
    class Runtime;

    void BindNativeRuntime(Runtime* runtime);
}

extern "C"
{
    // Runtime

    int Anjean_Runtime_CreateGameObject(
        std::uint32_t* outGameObjectId
    );

    int Anjean_Runtime_CreateCamera(
        std::uint32_t* outCameraId
    );

    int Anjean_Runtime_SetCurrentCamera(
        std::uint32_t cameraId
    );

    int Anjean_Runtime_GetCurrentCamera(
        std::uint32_t* outCameraId
    );

    // GameObject

    int Anjean_GameObject_GetPosition(
        std::uint32_t gameObjectId,
        Anjean::Core::Vector3* outPosition
    );

    int Anjean_GameObject_SetPosition(
        std::uint32_t gameObjectId,
        Anjean::Core::Vector3 position
    );

    int Anjean_GameObject_SetTexture(
        std::uint32_t gameObjectId,
        const char* filename,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t channels
    );

    int Anjean_GameObject_SetMesh(
        std::uint32_t gameObjectId,
        std::uint32_t meshId
    );

    int Anjean_GameObject_SetPhysicsBody(
        std::uint32_t gameObjectId,
        std::uint32_t physicsBodyId
    );

    int Anjean_GameObject_GetPhysicsBody(
        std::uint32_t gameObjectId,
        std::uint32_t* outPhysicsBodyId
    );

    // PhysicsBody

    int Anjean_PhysicsBody_Create(
        std::uint32_t physicsBodyType,
        std::uint32_t* outPhysicsBodyId
    );

    int Anjean_PhysicsBody_GetType(
        std::uint32_t physicsBodyId,
        std::uint32_t* outPhysicsBodyType
    );

    int Anjean_PhysicsBody_SetVelocity(
        std::uint32_t physicsBodyId,
        Anjean::Core::Vector3 velocity
    );

    int Anjean_PhysicsBody_GetVelocity(
        std::uint32_t physicsBodyId,
        Anjean::Core::Vector3* outVelocity
    );

    int Anjean_PhysicsBody_SetForce(
        std::uint32_t physicsBodyId,
        Anjean::Core::Vector3 force
    );

    int Anjean_PhysicsBody_GetForce(
        std::uint32_t physicsBodyId,
        Anjean::Core::Vector3* outForce
    );

    int Anjean_PhysicsBody_SetMass(
        std::uint32_t physicsBodyId,
        float mass
    );

    int Anjean_PhysicsBody_GetMass(
        std::uint32_t physicsBodyId,
        float* outMass
    );

    // Colliders

    int Anjean_PhysicsBody_GetColliderCount(
        std::uint32_t physicsBodyId,
        int* outCount
    );

    int Anjean_PhysicsBody_GetColliderAt(
        std::uint32_t physicsBodyId,
        int index,
        std::uint32_t* outColliderType,
        std::uint32_t* outColliderId
    );

    int Anjean_PhysicsBody_AddSphereCollider(
        std::uint32_t physicsBodyId,
        Anjean::Core::Vector3 localCenter,
        float radius,
        std::uint32_t* outColliderId
    );

    int Anjean_PhysicsBody_RemoveCollider(
        std::uint32_t physicsBodyId,
        std::uint32_t colliderId
    );

    // SphereCollider

    int Anjean_SphereCollider_GetLocalCenter(
        std::uint32_t physicsBodyId,
        std::uint32_t colliderId,
        Anjean::Core::Vector3* outLocalCenter
    );

    int Anjean_SphereCollider_SetLocalCenter(
        std::uint32_t physicsBodyId,
        std::uint32_t colliderId,
        Anjean::Core::Vector3 localCenter
    );

    int Anjean_SphereCollider_GetRadius(
        std::uint32_t physicsBodyId,
        std::uint32_t colliderId,
        float* outRadius
    );

    int Anjean_SphereCollider_SetRadius(
        std::uint32_t physicsBodyId,
        std::uint32_t colliderId,
        float radius
    );

    int Anjean_PhysicsBody_AddRectangularPrismCollider(
      std::uint32_t physicsBodyId,
      Anjean::Core::Vector3 localCenter,
      Anjean::Core::Vector3 halfExtents,
      std::uint32_t* outColliderId
    );

    int Anjean_RectangularPrismCollider_GetLocalCenter(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3* outLocalCenter
);

int Anjean_RectangularPrismCollider_SetLocalCenter(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3 localCenter
);

int Anjean_RectangularPrismCollider_GetHalfExtents(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3* outHalfExtents
);

int Anjean_RectangularPrismCollider_SetHalfExtents(
    std::uint32_t physicsBodyId,
    std::uint32_t colliderId,
    Anjean::Core::Vector3 halfExtents
);

    // Camera

    int Anjean_Camera_SetFieldOfView(
        std::uint32_t cameraId,
        float fieldOfView
    );

    int Anjean_Camera_SetNearClippingPlane(
        std::uint32_t cameraId,
        float nearClippingPlane
    );

    int Anjean_Camera_SetFarClippingPlane(
        std::uint32_t cameraId,
        float farClippingPlane
    );

    int Anjean_Camera_SetAspectRatio(
        std::uint32_t cameraId,
        float aspectRatio
    );

    // Input

    int Anjean_Input_IsKeyDown(
        int keyCode
    );
}