namespace Anjean;

public abstract class Collider
{
    internal uint PhysicsBodyId { get; }
    public uint Id { get; }

    internal Collider(uint physicsBodyId, uint colliderId)
    {
        PhysicsBodyId = physicsBodyId;
        Id = colliderId;
    }
}