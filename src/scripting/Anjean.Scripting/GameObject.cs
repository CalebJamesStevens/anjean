namespace Anjean;

public abstract class GameObject
{
    protected GameObject() {}

    internal InternalGameObject InternalGameObject { get; private set; } = null!;

    public Transform Transform { get; private set; } = null!;

    public Mesh? Mesh { get; private set; }
    public Texture? Texture { get; private set; }

    public PhysicsBody? PhysicsBody { get; private set; }

    public class Props
    {
        protected Props() {}

        public static readonly PropKey<Vec3> TransformPosition =
            new("GameObject.TransformPosition");
        
        public static readonly PropKey<Vec3> TransformRotation =
            new("GameObject.TransformRotation");

        public static readonly PropKey<Mesh?> Mesh =
            new("GameObject.Mesh");

        public static readonly PropKey<Texture?> Texture =
            new("GameObject.Texture");

        public static readonly PropKey<PhysicsBodyType?> PhysicsBodyType =
            new("GameObject.PhysicsBodyType");
    }

    public Dictionary<IPropKey, object?> PropValues { get; private set; } = new()
    {
        [Props.TransformPosition] = new Vec3(0, 0, 0)
    };

    internal void Attach(Dictionary<IPropKey, object?>? props = null)
    {
        if (props is not null)
        {
            PropValues = props;
        }

        InternalGameObject = CreateInternalObject();
        Transform = new Transform(InternalGameObject);

        Transform.Position = GetProp(
            Props.TransformPosition,
            new Vec3(0, 0, 0)
        );
        
        Transform.Rotation = GetProp(
            Props.TransformRotation,
            new Vec3(0, 0, 0)
        );

        Mesh = GetProp<Mesh?>(Props.Mesh, null);

        if (Mesh is not null)
        {
            InternalGameObject.SetMesh(Mesh.Id);
        }

        Texture = GetProp<Texture?>(Props.Texture, null);

        if (Texture is not null)
        {
            InternalGameObject.SetTexture(Texture);
        }

        PhysicsBodyType? physicsBodyType = GetProp<PhysicsBodyType?>(
            Props.PhysicsBodyType,
            null
        );

        if (physicsBodyType is not null)
        {
            PhysicsBody = CreateAndLinkPhysicsBody(physicsBodyType.Value);
        }

        ApplyProps();
    }

    public PhysicsBody CreateAndLinkPhysicsBody(PhysicsBodyType type)
    {
        int createRc = Native.Anjean_PhysicsBody_Create(
            type,
            out uint physicsBodyId
        );

        if (createRc != 0)
        {
            throw new InvalidOperationException(
                $"Failed to create physics body. rc={createRc}"
            );
        }

        int linkRc = Native.Anjean_GameObject_SetPhysicsBody(
            InternalGameObject.Id,
            physicsBodyId
        );

        if (linkRc != 0)
        {
            throw new InvalidOperationException(
                $"Failed to link physics body. rc={linkRc}"
            );
        }

        return new PhysicsBody(physicsBodyId);
    }

    public void LinkPhysicsBody(PhysicsBody physicsBody)
    {
        int rc = Native.Anjean_GameObject_SetPhysicsBody(
            InternalGameObject.Id,
            physicsBody.Id
        );

        if (rc != 0)
        {
            throw new InvalidOperationException(
                $"Failed to link physics body. rc={rc}"
            );
        }

        PhysicsBody = physicsBody;
    }

  public void KinematicMove(PhysicsBody physicsBody, Vec3 displacement)
  {
    int rc = Native.Anjean_GameObject_KinematicMove(
      InternalGameObject.Id,
      physicsBody.Id,
      displacement
    );

    if (rc != 0)
    {
        throw new InvalidOperationException(
            $"Failed to move kinematic physics body. rc={rc}"
        );
    }
  }

    internal virtual InternalGameObject CreateInternalObject()
    {
        int rc = Native.Anjean_Runtime_CreateGameObject(out uint id);

        if (rc != 0)
        {
            throw new InvalidOperationException(
                $"Failed to create native game object. rc={rc}"
            );
        }

        return new InternalGameObject(id);
    }

    protected virtual void ApplyProps() {}

    protected T GetProp<T>(PropKey<T> key, T fallback)
    {
        if (!PropValues.TryGetValue(key, out object? value))
        {
            return fallback;
        }

        if (value is not T typedValue)
        {
            return fallback;
        }

        return typedValue;
    }

    public virtual void Start() {}
    public virtual void Update() {}
    public virtual void PhysicsUpdate(float deltaTime) {}
}