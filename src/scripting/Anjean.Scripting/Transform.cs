namespace Anjean;

public class Transform
{
    private readonly InternalGameObject gameObject;

    internal Transform(InternalGameObject gameObject)
    {
        this.gameObject = gameObject;
    }

    public Vec3 Position
    {
        get => gameObject.Position;
        set => gameObject.Position = value;
    }
    
    public Vec3 Rotation
    {
        get => gameObject.Rotation;
        set => gameObject.Rotation = value;
    }
}