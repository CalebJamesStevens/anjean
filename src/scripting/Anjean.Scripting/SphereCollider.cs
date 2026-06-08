namespace Anjean;

public sealed class SphereCollider : Collider
{
    internal SphereCollider(uint physicsBodyId, uint colliderId)
        : base(physicsBodyId, colliderId)
    {
    }

    public float Radius
    {
        get
        {
            int rc = Native.Anjean_SphereCollider_GetRadius(
                PhysicsBodyId,
                Id,
                out float radius
            );

            if (rc != 0)
            {
                throw new InvalidOperationException($"Failed to get sphere radius. rc={rc}");
            }

            return radius;
        }

        set
        {
            int rc = Native.Anjean_SphereCollider_SetRadius(
                PhysicsBodyId,
                Id,
                value
            );

            if (rc != 0)
            {
                throw new InvalidOperationException($"Failed to set sphere radius. rc={rc}");
            }
        }
    }
}