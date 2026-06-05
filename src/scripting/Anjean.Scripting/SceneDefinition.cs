namespace Anjean;

public abstract class SceneDefinition
{
    public virtual SceneObjectDefinition[] StartUp()
    {
        return [];
    }

    protected static SceneObjectDefinition DefineObject<TGameObject>(
        Dictionary<GameObject.PropsNames, object> props
    )
        where TGameObject : GameObject
    {
        return new SceneObjectDefinition(typeof(TGameObject), props);
    }
}