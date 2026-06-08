namespace Anjean;

public sealed class PhysicsBody
{
    internal uint Id { get; }

    public ColliderCollection Colliders { get; }

    internal PhysicsBody(uint physicsBodyId)
    {
        Id = physicsBodyId;
        Colliders = new ColliderCollection(physicsBodyId);
    }

    public PhysicsBodyType Type
    {
        get
        {
            int rc = Native.Anjean_PhysicsBody_GetType(Id, out PhysicsBodyType type);
            if (rc != 0) throw new InvalidOperationException($"Failed to get body type. rc={rc}");
            return type;
        }
    }

    public Vec3 Velocity
    {
        get
        {
            int rc = Native.Anjean_PhysicsBody_GetVelocity(Id, out Vec3 velocity);
            if (rc != 0) throw new InvalidOperationException($"Failed to get velocity. rc={rc}");
            return velocity;
        }

        set
        {
            int rc = Native.Anjean_PhysicsBody_SetVelocity(Id, value);
            if (rc != 0) throw new InvalidOperationException($"Failed to set velocity. rc={rc}");
        }
    }

    public Vec3 Force
    {
        get
        {
            int rc = Native.Anjean_PhysicsBody_GetForce(Id, out Vec3 force);
            if (rc != 0) throw new InvalidOperationException($"Failed to get force. rc={rc}");
            return force;
        }

        set
        {
            int rc = Native.Anjean_PhysicsBody_SetForce(Id, value);
            if (rc != 0) throw new InvalidOperationException($"Failed to set force. rc={rc}");
        }
    }

    public float Mass
    {
        get
        {
            int rc = Native.Anjean_PhysicsBody_GetMass(Id, out float mass);
            if (rc != 0) throw new InvalidOperationException($"Failed to get mass. rc={rc}");
            return mass;
        }

        set
        {
            int rc = Native.Anjean_PhysicsBody_SetMass(Id, value);
            if (rc != 0) throw new InvalidOperationException($"Failed to set mass. rc={rc}");
        }
    }
}