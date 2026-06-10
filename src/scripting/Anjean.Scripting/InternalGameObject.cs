namespace Anjean;

internal class InternalGameObject
{
    protected readonly uint id;

    internal uint Id => id;

    internal InternalGameObject(uint id)
    {
        this.id = id;
    }

    internal void SetMesh(uint meshId)
    {
        int rc = Native.Anjean_GameObject_SetMesh(id, meshId);

        if (rc != 0)
        {
            throw new InvalidOperationException($"Failed to set mesh. rc={rc}");
        }
    }

    internal void SetTexture(Texture texture)
    {
        int rc = Native.Anjean_GameObject_SetTexture(
            id,
            texture.Filename,
            texture.Width,
            texture.Height,
            texture.Channels
        );

        if (rc != 0)
        {
            throw new InvalidOperationException($"Failed to set texture. rc={rc}");
        }
    }

    internal Vec3 Position
    {
        get
        {
            int rc = Native.Anjean_GameObject_GetPosition(id, out Vec3 position);

            if (rc != 0)
            {
                throw new InvalidOperationException($"Failed to get position. rc={rc}");
            }

            return position;
        }

        set
        {
            int rc = Native.Anjean_GameObject_SetPosition(id, value);

            if (rc != 0)
            {
                throw new InvalidOperationException($"Failed to set position. rc={rc}");
            }
        }
    }
    internal Vec3 Rotation
    {
        get
        {
            int rc = Native.Anjean_GameObject_GetRotation(id, out Vec3 rotation);

            if (rc != 0)
            {
                throw new InvalidOperationException($"Failed to get rotation. rc={rc}");
            }

            return rotation;
        }

        set
        {
            int rc = Native.Anjean_GameObject_SetRotation(id, value);

            if (rc != 0)
            {
                throw new InvalidOperationException($"Failed to set rotation. rc={rc}");
            }
        }
    }
}