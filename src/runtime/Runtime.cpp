#include "Runtime.h"
#include "objects/GameObject.h"
#include "objects/Camera.h"
#include "scripting/ScriptingEngine.h"
#include "scripting/NativeBindings.h"
#include <cstdlib>

namespace Anjean::Runtime
{

    Runtime::Runtime()
    {
        inputManager = InputManager();

        scriptingEngine.bindRuntime(this);
        BindNativeRuntime(this);

        setenv(
            "ANJEAN_NATIVE_LIBRARY",
            "/Users/caleb/repos/anjean/build/libAnjean.Native.dylib",
            1
        );

        namespace fs = std::filesystem;

        fs::path projectRoot = fs::current_path();

        fs::path anjeanDir = projectRoot / ".anjean";
        fs::path sdkDir = anjeanDir / "sdk";

        fs::path runtimeConfig =
            sdkDir / "Anjean.Scripting.runtimeconfig.json";

        fs::path scriptingDll =
            sdkDir / "Anjean.Scripting.dll";

        fs::path gameAssembly =
            anjeanDir / "bin" / "Debug" / "net8.0" /
            (projectRoot.filename().string() + ".dll");

        if (!fs::exists(runtimeConfig))
        {
            throw std::runtime_error(
                "Missing runtime config: " + runtimeConfig.string()
            );
        }

        if (!fs::exists(scriptingDll))
        {
            throw std::runtime_error(
                "Missing scripting DLL: " + scriptingDll.string()
            );
        }

        if (!fs::exists(gameAssembly))
        {
            throw std::runtime_error(
                "Missing game assembly: " + gameAssembly.string() +
                "\nRun dotnet build first."
            );
        }

        scriptingEngine.load(
            runtimeConfig.string(),
            scriptingDll.string()
        );

        scriptingEngine.loadGameAssembly(
            gameAssembly.string()
        );

        fs::path currentPath = projectRoot;

        if (!std::filesystem::exists(currentPath / "Scenes/Main.cs"))
        {
            return;
        }

        scriptingEngine.startMainScene();
    }
    void Runtime::beginTick()
    {
        inputManager.updateInputState();
        inputManager.pollEvents();
    }

    void Runtime::executeTick()
    {
        scriptingEngine.updateAll();
    
    }
    void Runtime::executePhysicsTick(float deltaTime)
    {
        scriptingEngine.physicsUpdateAll(deltaTime);
    }


    void Runtime::endTick()
    {
        inputManager.frameEvents.clear();
    }

    std::vector<GameObject*> Runtime::getRenderableSceneObjects()
    {
        std::vector<GameObject*> renderables;

        for (auto& object : sceneObjects)
        {
            if (object->mesh.has_value())
            {
                renderables.push_back(object.get());
            }
        }

        return renderables;
    }
    
    std::vector<GameObject*> Runtime::getPhysicsAwareSceneObjects()
    {
        std::vector<GameObject*> physicsAwareObjects;

        for (auto& object : sceneObjects)
        {
            if (object->physicsBodyId.has_value())
            {
                physicsAwareObjects.push_back(object.get());
            }
        }

        return physicsAwareObjects;
    }

    PhysicsBody& Runtime::createPhysicsBody(Core::PhysicsBodyType type)
{
    auto body = std::make_unique<PhysicsBody>();

    body->id = nextPhysicsBodyId++;
    body->type = type;

    PhysicsBody& ref = *body;
    physicsBodies.push_back(std::move(body));

    return ref;
}

PhysicsBody& Runtime::getPhysicsBodyById(std::uint32_t physicsBodyId)
{
    for (auto& body : physicsBodies)
    {
        if (body->id == physicsBodyId)
        {
            return *body;
        }
    }

    throw std::runtime_error("PhysicsBody not found");
}

void Runtime::setGameObjectPhysicsBody(
    std::uint32_t gameObjectId,
    std::uint32_t physicsBodyId
)
{
    GameObject& object = getGameObjectById(gameObjectId);

    // Validate that the body actually exists.
    getPhysicsBodyById(physicsBodyId);

    object.physicsBodyId = physicsBodyId;
}

std::uint32_t Runtime::getGameObjectPhysicsBody(
    std::uint32_t gameObjectId
)
{
    GameObject& object = getGameObjectById(gameObjectId);

    if (!object.physicsBodyId.has_value())
    {
        throw std::runtime_error("GameObject has no PhysicsBody");
    }

    return object.physicsBodyId.value();
}
    
    Camera& Runtime::getCurrentCamera()
    {
        if (currentCameraId == 0)
        {
            throw std::runtime_error("No current camera has been set");
        }

        GameObject& object = getGameObjectById(currentCameraId);

        if (object.getGameObjectType() != ANJEAN_GAMEOBJECT_CAMERA)
        {
            throw std::runtime_error("Current camera ID is not a camera");
        }

        return static_cast<Camera&>(object);
    }

    std::vector<Mesh> Runtime::getAllMeshes() {
      float lw = .25f / 6.0f;
      float lh = .25f / 6.0f;
      float ld = 1.0f / 2.0f;
      float hw = 41.13f / 2.0f;
      float hh = 15.98f / 2.0f;
      float hd = 1.0f / 2.0f;
      decltype(Mesh::vertices) cubeVertices;
      decltype(Mesh::vertices) mapVerts;
      
      float u0 = 0.0f;
      float u1 = 1.0f;
      float v0 = 0.0f;
      float v1 = 1.0f;

      cubeVertices = {
          // Front, +Z
          { { -lw, -lh,  ld }, { u0, v1 } },
          { {  lw, -lh,  ld }, { u1, v1 } },
          { { -lw,  lh,  ld }, { u0, v0 } },

          { {  lw, -lh,  ld }, { u1, v1 } },
          { {  lw,  lh,  ld }, { u1, v0 } },
          { { -lw,  lh,  ld }, { u0, v0 } },
      };
      std::vector<Mesh> meshes;
      Mesh cube;
      cube.id = 1;
      cube.vertexCount = 6;
      cube.vertices = cubeVertices;
      
      mapVerts = {
          // Front, +Z
          { { -hw, -hh,  hd }, { u0, v1 } },
          { {  hw, -hh,  hd }, { u1, v1 } },
          { { -hw,  hh,  hd }, { u0, v0 } },

          { {  hw, -hh,  hd }, { u1, v1 } },
          { {  hw,  hh,  hd }, { u1, v0 } },
          { { -hw,  hh,  hd }, { u0, v0 } },

          // Back, -Z
          { {  hw, -hh, -hd }, { u0, v1 } },
          { { -hw, -hh, -hd }, { u1, v1 } },
          { {  hw,  hh, -hd }, { u0, v0 } },

          { { -hw, -hh, -hd }, { u1, v1 } },
          { { -hw,  hh, -hd }, { u1, v0 } },
          { {  hw,  hh, -hd }, { u0, v0 } },

          // Top, +Y
          { { -hw,  hh,  hd }, { u0, v1 } },
          { {  hw,  hh,  hd }, { u1, v1 } },
          { { -hw,  hh, -hd }, { u0, v0 } },

          { {  hw,  hh,  hd }, { u1, v1 } },
          { {  hw,  hh, -hd }, { u1, v0 } },
          { { -hw,  hh, -hd }, { u0, v0 } },

          // Bottom, -Y
          { { -hw, -hh, -hd }, { u0, v1 } },
          { {  hw, -hh, -hd }, { u1, v1 } },
          { { -hw, -hh,  hd }, { u0, v0 } },

          { {  hw, -hh, -hd }, { u1, v1 } },
          { {  hw, -hh,  hd }, { u1, v0 } },
          { { -hw, -hh,  hd }, { u0, v0 } },

          // Left, -X
          { { -hw, -hh,  hd }, { u0, v1 } },
          { { -hw,  hh,  hd }, { u0, v0 } },
          { { -hw, -hh, -hd }, { u1, v1 } },

          { { -hw,  hh,  hd }, { u0, v0 } },
          { { -hw,  hh, -hd }, { u1, v0 } },
          { { -hw, -hh, -hd }, { u1, v1 } },

          // Right, +X
          { {  hw, -hh, -hd }, { u0, v1 } },
          { {  hw,  hh, -hd }, { u0, v0 } },
          { {  hw, -hh,  hd }, { u1, v1 } },

          { {  hw,  hh, -hd }, { u0, v0 } },
          { {  hw,  hh,  hd }, { u1, v0 } },
          { {  hw, -hh,  hd }, { u1, v1 } },
      };
      Mesh cube2;
      cube2.id = 2;
      cube2.vertexCount = 36;
      cube2.vertices = mapVerts;

      meshes.push_back(cube);
      meshes.push_back(cube2);

      return meshes;
    }
    
    std::vector<Texture> Runtime::getAllTextures() {
      std::vector<Texture> textures;
      
      Texture tempTexture{};
      tempTexture.filename="text.png";
      tempTexture.width=36;
      tempTexture.height=36;
      tempTexture.channels=0;
      
      textures.push_back(tempTexture);
      
      Texture tempTexture2{};
      tempTexture2.filename="image.png";
      tempTexture2.width=36;
      tempTexture2.height=36;
      tempTexture2.channels=0;
      
      textures.push_back(tempTexture2);
      
      Texture tempTexture3{};
      tempTexture3.filename="LinkSprite.png";
      tempTexture3.width=16;
      tempTexture3.height=16;
      tempTexture3.channels=4;
      
      textures.push_back(tempTexture3);

      return textures;
    }

    GameObject& Runtime::createGameObject()
    {
        auto object = std::make_unique<GameObject>();
        object->id = nextGameObjectId++;

        GameObject& ref = *object;
        sceneObjects.push_back(std::move(object));

        return ref;
    }

    GameObject& Runtime::getGameObjectById(std::uint32_t id)
    {
        for (auto& object : sceneObjects)
        {
            if (object->id == id)
            {
                return *object;
            }
        }

        throw std::runtime_error("GameObject not found");
    }

    void Runtime::setCurrentCamera(std::uint32_t cameraId)
    {
        currentCameraId = cameraId;
    }

    Camera& Runtime::createCamera()
    {
        auto camera = std::make_unique<Camera>();
        camera->id = nextGameObjectId++;

        Camera& ref = *camera;
        sceneObjects.push_back(std::move(camera));

        return ref;
    }
}