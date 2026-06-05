#pragma once

#include "../runtime/Runtime.h"
#include "../runtime/RuntimeTypes.h"
#include "../runtime/objects/GameObject.h"
#include "../rendering/Renderer.h"
#include "../Core/Core.h"

namespace Anjean::Orchestrator {

  struct RendererState {
    Rendering::PipelineHandle spritePipelineHandle;
    std::unordered_map<decltype(Runtime::Mesh::id), Core::MeshData> runtimeRendererMeshMap;
    // std::unordered_map<decltype(Runtime::GameObject::id), Rendering::TextureHandle> runtimeGameObjectRendererTextureHandleMap;
    std::unordered_map<decltype(Runtime::Texture::filename), Rendering::TextureHandle> runtimeTextureRendererTextureHandleMap;
  };

  class Orchestrator{
    public:
      Orchestrator();
      Anjean::Runtime::Runtime* runtime;
      Anjean::Rendering::Renderer* renderer;
      std::vector<Runtime::GameObject> gameObjectsToRender;
      Runtime::Camera currentCamera;
      RendererState renderState;
      void GetRendererData();
      void GetRuntimeData();
      void PostToRuntime();
      void PostToRenderer();
      void HandleSDLEvent(const SDL_Event& event);
      void Tick();
  };
}