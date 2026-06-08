namespace Anjean;

public sealed class RectangularPrismCollider : Collider
{
    internal RectangularPrismCollider(uint physicsBodyId, uint colliderId)
        : base(physicsBodyId, colliderId)
    {
    }

    public Vec3 LocalCenter
    {
        get
        {
            int rc = Native.Anjean_RectangularPrismCollider_GetLocalCenter(
                PhysicsBodyId,
                Id,
                out Vec3 localCenter
            );

            if (rc != 0)
            {
                throw new InvalidOperationException(
                    $"Failed to get rectangular prism local center. rc={rc}"
                );
            }

            return localCenter;
        }

        set
        {
            int rc = Native.Anjean_RectangularPrismCollider_SetLocalCenter(
                PhysicsBodyId,
                Id,
                value
            );

            if (rc != 0)
            {
                throw new InvalidOperationException(
                    $"Failed to set rectangular prism local center. rc={rc}"
                );
            }
        }
    }

    public Vec3 HalfExtents
    {
        get
        {
            int rc = Native.Anjean_RectangularPrismCollider_GetHalfExtents(
                PhysicsBodyId,
                Id,
                out Vec3 halfExtents
            );

            if (rc != 0)
            {
                throw new InvalidOperationException(
                    $"Failed to get rectangular prism half extents. rc={rc}"
                );
            }

            return halfExtents;
        }

        set
        {
            int rc = Native.Anjean_RectangularPrismCollider_SetHalfExtents(
                PhysicsBodyId,
                Id,
                value
            );

            if (rc != 0)
            {
                throw new InvalidOperationException(
                    $"Failed to set rectangular prism half extents. rc={rc}"
                );
            }
        }
    }

    public Vec3 Size
    {
        get => HalfExtents * 2.0f;
        set => HalfExtents = value / 2.0f;
    }    
}