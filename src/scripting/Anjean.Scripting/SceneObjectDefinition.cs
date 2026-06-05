namespace Anjean;

public sealed record SceneObjectDefinition(
    Type ObjectType,
    Dictionary<GameObject.PropsNames, object> Props
);

public static class SceneObject
{
    public static SceneObjectDefinition DefineObject<TGameObject>(
        Dictionary<GameObject.PropsNames, object> props
    )
        where TGameObject : GameObject
    {
        return new SceneObjectDefinition(typeof(TGameObject), props);
    }
}