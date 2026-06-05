using System.Runtime;

namespace Anjean;

public abstract class GameObject
{
    protected GameObject() {}

    internal InternalGameObject InternalGameObject { get; private set; } = null!;

    public Transform Transform { get; private set; } = null!;

    public Mesh? Mesh { get; set; } = null;

    public Texture? Texture { get; set; } = null;

    public enum PropsNames
    {
        TransformPosition,
        Mesh,
        Texture
    }
    public Dictionary<PropsNames, object> Props { get; set; } = new(){
      [PropsNames.TransformPosition] = new Vec3(0, 0, 0)
    };

    internal void Attach(uint id, Dictionary<PropsNames, object>? props = null)
    {
        if (props is not null)
        {
            Props = props;
        }

        InternalGameObject = new InternalGameObject(id);
        Transform = new Transform(InternalGameObject);

        Transform.Position = GetProp(
            PropsNames.TransformPosition,
            new Vec3(0, 0, 0)
        );

        Mesh? mesh = GetProp<Mesh?>(
            PropsNames.Mesh,
            null
        );

        if (mesh is not null)
        {
            InternalGameObject.SetMesh(mesh.Id);
        }

        Texture? texture = GetProp<Texture?>(
            PropsNames.Texture,
            null
        );

        if (texture is not null)
        {
            InternalGameObject.SetTexture(texture);
        }
    }

    protected T GetProp<T>(PropsNames name, T fallback)
    {
        if (!Props.TryGetValue(name, out object? value))
        {
            return fallback;
        }

        if (value is not T typedValue)
        {
            return fallback;
        }

        return typedValue;
    }

    public virtual void AssignProps() {}

    public virtual void Start() {}
    public virtual void Update() {}
}