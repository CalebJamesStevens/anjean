using System.Runtime.InteropServices;

namespace Anjean;

[StructLayout(LayoutKind.Sequential)]
public struct Position3D
{
    public float X;
    public float Y;
    public float Z;

    public Position3D(float x, float y, float z)
    {
        X = x;
        Y = y;
        Z = z;
    }
}

[StructLayout(LayoutKind.Sequential)]
public struct Position2D
{
    public float X;
    public float Y;

    public Position2D(float x, float y)
    {
        X = x;
        Y = y;
    }
}

[StructLayout(LayoutKind.Sequential)]
public struct MeshVertex
{
    public Position3D Position;
    public Position2D Uv;

    public MeshVertex(Position3D position, Position2D uv)
    {
        Position = position;
        Uv = uv;
    }
}

public class MeshData
{
    public uint Id { get; set; } = 0;

    public List<MeshVertex> Vertices { get; set; } = new();
}