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
        currentCamera = Camera();
        currentCamera.transform.position = {0,0,0};
        currentCamera.transform.rotation = {0,0,0};
        currentCamera.transform.scale = {1,1,1};
        sceneObjects = std::vector<GameObject>{};
        scriptingEngine.bindRuntime(this);
        BindNativeRuntime(this);
        setenv(
            "ANJEAN_NATIVE_LIBRARY",
            "/Users/caleb/repos/anjean/build/libAnjean.Native.dylib",
            1
        );
        scriptingEngine.load(
          "/Users/caleb/repos/game-test/TestGame/.anjean/Anjean.Scripting.runtimeconfig.json",
          "/Users/caleb/repos/game-test/TestGame/.anjean/Anjean.Scripting.dll"
        );

        scriptingEngine.loadGameAssembly(
            "/Users/caleb/repos/game-test/TestGame/.anjean/bin/Debug/net8.0/TestGame.dll"
        );
        
        scriptingEngine.loadGameAssembly(
            "/Users/caleb/repos/game-test/TestGame/.anjean/bin/Debug/net8.0/TestGame.dll"
        );

        std::filesystem::path currentPath = std::filesystem::current_path();

        if (!std::filesystem::exists(currentPath.concat("/Scenes/Main.cs"))) {
          return;
        }

        scriptingEngine.startMainScene();
        // GameObject& player = createGameObject();
        // scriptingEngine.createGameObjectScript("Player", player.id);
        // player.transform.position.x = 0;
        // player.transform.position.y = 0;
        // player.transform.position.z = -2;
        // player.mesh = Mesh();
        // player.mesh.value().id = getAllMeshes().at(0).id;
        // player.mesh.value().vertexCount = getAllMeshes().at(0).vertexCount;
        // player.mesh.value().vertices = getAllMeshes().at(0).vertices;
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


    void Runtime::endTick()
    {
        inputManager.frameEvents.clear();
    }

    std::vector<GameObject> Runtime::getRenderableSceneObjects() {
      std::vector<GameObject> renderables;
      for (GameObject go : sceneObjects) {
        if (go.mesh.has_value()) {
          renderables.push_back(go);
        }
      }
      return renderables;
    }
    
    Camera Runtime::getCurrentCamera() {
      return currentCamera;
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
        GameObject object{};
        object.id = static_cast<std::uint32_t>(sceneObjects.size() + 1);

        sceneObjects.emplace_back(object);
        return sceneObjects.back();
    }

    GameObject& Runtime::getGameObjectById(std::uint32_t id)
    {
        for (auto& object : sceneObjects)
        {
            if (object.id == id)
            {
                return object;
            }
        }

        throw std::runtime_error("GameObject not found");
    }
}