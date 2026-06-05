#include "NativeBindings.h"

#include "../Runtime.h"
#include "../RuntimeTypes.h"
#include "../objects/GameObject.h"
#include "../../Core/Core.h"

namespace
{
    Anjean::Runtime::Runtime* g_runtime = nullptr;

    constexpr int ANJEAN_OK = 0;
    constexpr int ANJEAN_ERR_NO_RUNTIME = -1;
    constexpr int ANJEAN_ERR_NULL_ARGUMENT = -2;
    constexpr int ANJEAN_ERR_GAME_OBJECT_NOT_FOUND = -3;
    constexpr int ANJEAN_ERR_UNKNOWN = -999;
}

namespace Anjean::Runtime
{
    void BindNativeRuntime(Runtime* runtime)
    {
        g_runtime = runtime;
    }
}

extern "C"
{
  int Anjean_Runtime_CreateGameObject(std::uint32_t* outGameObjectId)
  {
    if (!g_runtime)
    {
        return -1;
    }

    if (!outGameObjectId)
    {
        return -2;
    }

    try
    {
        auto& object = g_runtime->createGameObject();
        *outGameObjectId = object.id;

        return 0;
    }
    catch (...)
    {
        return -999;
    }
  }


int Anjean_GameObject_GetPosition(
    std::uint32_t gameObjectId,
    AnjeanVec3* outPosition
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    if (!outPosition)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        outPosition->x = object.transform.position.x;
        outPosition->y = object.transform.position.y;
        outPosition->z = object.transform.position.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_GameObject_SetPosition(
    std::uint32_t gameObjectId,
    AnjeanVec3 position
)
{
    if (!g_runtime)
    {
        return ANJEAN_ERR_NO_RUNTIME;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        object.transform.position.x = position.x;
        object.transform.position.y = position.y;
        object.transform.position.z = position.z;

        return ANJEAN_OK;
    }
    catch (const std::runtime_error&)
    {
        return ANJEAN_ERR_GAME_OBJECT_NOT_FOUND;
    }
    catch (...)
    {
        return ANJEAN_ERR_UNKNOWN;
    }
}

int Anjean_GameObject_SetMesh(
    std::uint32_t gameObjectId,
    std::uint32_t meshId
)
{
    if (!g_runtime)
    {
        return -1;
    }

    if (meshId == 0)
    {
        return ANJEAN_ERR_NULL_ARGUMENT;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        std::vector<Anjean::Runtime::Mesh> meshes = g_runtime->getAllMeshes();

        for (const Anjean::Runtime::Mesh& mesh : meshes)
        {
            if (mesh.id == meshId)
            {
                object.mesh = mesh;
                return ANJEAN_OK;
            }
        }

        return -4; // mesh id not found
    }
    catch (...)
    {
        return -999;
    }
  }

  int Anjean_GameObject_SetTexture(
    std::uint32_t gameObjectId,
    const char* filename,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t channels
)
{
    if (!g_runtime)
    {
        return -1;
    }

    if (!filename)
    {
        return -2;
    }

    try
    {
        auto& object = g_runtime->getGameObjectById(gameObjectId);

        Anjean::Runtime::Texture texture{};
        texture.filename = filename;
        texture.width = width;
        texture.height = height;
        texture.channels = channels;

        object.texture = texture;

        return 0;
    }
    catch (...)
    {
        return -999;
    }
  }

  int32_t Anjean_Input_IsKeyDown(int32_t keyCode)
  {
      if (!g_runtime)
      {
          return 0;
      }

      auto key = static_cast<Anjean::Runtime::Key>(keyCode);

      if (key <= Anjean::Runtime::Key::Unknown ||
          key >= Anjean::Runtime::Key::Count)
      {
          return 0;
      }

      return g_runtime->inputManager.isKeyDown(key) ? 1 : 0;
  }

}