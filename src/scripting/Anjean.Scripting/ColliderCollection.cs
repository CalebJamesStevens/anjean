namespace Anjean;

public sealed class ColliderCollection
{
    private readonly uint physicsBodyId;

    internal ColliderCollection(uint physicsBodyId)
    {
        this.physicsBodyId = physicsBodyId;
    }

    public int Count
    {
        get
        {
            int rc = Native.Anjean_PhysicsBody_GetColliderCount(
                physicsBodyId,
                out int count
            );

            if (rc != 0)
            {
                throw new InvalidOperationException($"Failed to get collider count. rc={rc}");
            }

            return count;
        }
    }

    public Collider this[int index]
    {
        get
        {
            int rc = Native.Anjean_PhysicsBody_GetColliderAt(
                physicsBodyId,
                index,
                out ColliderType type,
                out uint colliderId
            );

            if (rc != 0)
            {
                throw new InvalidOperationException($"Failed to get collider. rc={rc}");
            }

            return CreateColliderProxy(physicsBodyId, colliderId, type);
        }
    }

    public List<Collider> GetAll()
    {
        List<Collider> colliders = new();

        for (int i = 0; i < Count; i++)
        {
            colliders.Add(this[i]);
        }

        return colliders;
    }

    public SphereCollider AddSphere(Vec3 localCenter, float radius)
    {
        int rc = Native.Anjean_PhysicsBody_AddSphereCollider(
            physicsBodyId,
            localCenter,
            radius,
            out uint colliderId
        );

        if (rc != 0)
        {
            throw new InvalidOperationException($"Failed to add sphere collider. rc={rc}");
        }

        return new SphereCollider(physicsBodyId, colliderId);
    }

    public RectangularPrismCollider AddRectangularPrism(
    Vec3 localCenter,
    Vec3 halfExtents
    )
    {
        int rc = Native.Anjean_PhysicsBody_AddRectangularPrismCollider(
            physicsBodyId,
            localCenter,
            halfExtents,
            out uint colliderId
        );

        if (rc != 0)
        {
            throw new InvalidOperationException(
                $"Failed to add rectangular prism collider. rc={rc}"
            );
        }

        return new RectangularPrismCollider(physicsBodyId, colliderId);
    }

    private static Collider CreateColliderProxy(
        uint physicsBodyId,
        uint colliderId,
        ColliderType type
    )
    {
        return type switch
        {
            ColliderType.Sphere => new SphereCollider(physicsBodyId, colliderId),
            ColliderType.RectangularPrism => new RectangularPrismCollider(physicsBodyId, colliderId),
            // ColliderType.Capsule => new CapsuleCollider(physicsBodyId, colliderId),

            _ => throw new InvalidOperationException($"Unknown collider type: {type}")
        };
    }
}