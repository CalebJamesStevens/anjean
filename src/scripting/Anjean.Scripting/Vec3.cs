using System.Runtime.InteropServices;

namespace Anjean;

[StructLayout(LayoutKind.Sequential)]
public struct Vec3
{
    public float X;
    public float Y;
    public float Z;

    public Vec3(float x, float y, float z)
    {
        X = x;
        Y = y;
        Z = z;
    }

    public static Vec3 operator +(Vec3 a, Vec3 b)
    {
        return new Vec3(
            a.X + b.X,
            a.Y + b.Y,
            a.Z + b.Z
        );
    }

    public static Vec3 operator -(Vec3 a, Vec3 b)
    {
        return new Vec3(
            a.X - b.X,
            a.Y - b.Y,
            a.Z - b.Z
        );
    }

    public static Vec3 operator *(Vec3 v, float scalar)
    {
        return new Vec3(
            v.X * scalar,
            v.Y * scalar,
            v.Z * scalar
        );
    }

    public static Vec3 operator *(float scalar, Vec3 v)
    {
        return v * scalar;
    }

    public static Vec3 operator /(Vec3 v, float scalar)
    {
        return new Vec3(
            v.X / scalar,
            v.Y / scalar,
            v.Z / scalar
        );
    }

    public override string ToString()
    {
        return $"Vec3({X}, {Y}, {Z})";
    }
}