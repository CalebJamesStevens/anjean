using System.Runtime.InteropServices;

namespace Anjean;

internal static class Native
{
    [DllImport("Anjean.Native", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int Anjean_Runtime_CreateGameObject(out uint gameObjectId);

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
    internal static extern int Anjean_Input_IsKeyDown(KeyCode keyCode);
}