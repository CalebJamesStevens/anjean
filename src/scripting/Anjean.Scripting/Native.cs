using System.Runtime.InteropServices;

namespace Anjean;

internal static class Native
{
    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Runtime_CreateGameObject(out uint gameObjectId);

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Runtime_CreateCamera(out uint cameraId);

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Runtime_SetCurrentCamera(uint cameraId);
    
    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Runtime_GetCurrentCamera(out uint cameraId);

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_GetPosition(
        uint gameObjectId,
        out Vec3 outPosition
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_SetPosition(
        uint gameObjectId,
        Vec3 position
    );
    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_GetRotation(
        uint gameObjectId,
        out Vec3 outRotation
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_SetRotation(
        uint gameObjectId,
        Vec3 rotation
    );
    
    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_GetColliderCount(
        uint physicsBodyId,
        out int count
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_GetColliderAt(
        uint physicsBodyId,
        int index,
        out ColliderType type,
        out uint colliderId
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_GetVelocity(
        uint physicsBodyId,
        out Vec3 outVelocity
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_SetVelocity(
        uint physicsBodyId,
        Vec3 velocity
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_GetForce(
        uint physicsBodyId,
        out Vec3 outForce
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_SetForce(
        uint physicsBodyId,
        Vec3 force
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_GetMass(
        uint physicsBodyId,
        out float outMass
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_SetMass(
        uint physicsBodyId,
        float mass
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_Create(
        PhysicsBodyType type,
        out uint physicsBodyId
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_KinematicMove(
        uint gameObjectId,
        uint physicsBodyId,
        Vec3 displacement
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_SetPhysicsBody(
        uint gameObjectId,
        uint physicsBodyId
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_GetPhysicsBody(
        uint gameObjectId,
        out uint physicsBodyId
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_SetMesh(
        uint gameObjectId,
        uint meshId
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_GameObject_SetTexture(
        uint gameObjectId,

        [MarshalAs(UnmanagedType.LPUTF8Str)]
        string filename,

        uint width,
        uint height,
        uint channels
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Camera_SetFieldOfView(
        uint cameraId,
        float fieldOfView
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Camera_SetNearClippingPlane(
        uint cameraId,
        float nearClippingPlane
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Camera_SetFarClippingPlane(
        uint cameraId,
        float farClippingPlane
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Camera_SetAspectRatio(
        uint cameraId,
        float aspectRatio
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Input_IsKeyDown(KeyCode keyCode);

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_AddSphereCollider(
        uint physicsBodyId,
        Vec3 localCenter,
        float radius,
        out uint colliderId
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_SphereCollider_GetRadius(
        uint physicsBodyId,
        uint colliderId,
        out float radius
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_SphereCollider_SetRadius(
        uint physicsBodyId,
        uint colliderId,
        float radius
    );
    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_PhysicsBody_GetType(
        uint physicsBodyId,
        out PhysicsBodyType outType
    );

    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
internal static extern int Anjean_RectangularPrismCollider_GetLocalCenter(
    uint physicsBodyId,
    uint colliderId,
    out Vec3 outLocalCenter
);

[DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
internal static extern int Anjean_RectangularPrismCollider_SetLocalCenter(
    uint physicsBodyId,
    uint colliderId,
    Vec3 localCenter
);

[DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
internal static extern int Anjean_RectangularPrismCollider_GetHalfExtents(
    uint physicsBodyId,
    uint colliderId,
    out Vec3 outHalfExtents
);

[DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
internal static extern int Anjean_RectangularPrismCollider_SetHalfExtents(
    uint physicsBodyId,
    uint colliderId,
    Vec3 halfExtents
);
[DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
internal static extern int Anjean_PhysicsBody_AddRectangularPrismCollider(
    uint physicsBodyId,
    Vec3 localCenter,
    Vec3 halfExtents,
    out uint colliderId
);
}