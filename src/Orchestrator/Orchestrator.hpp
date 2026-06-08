#pragma once

#include <chrono>
#include "../runtime/Runtime.h"
#include "../runtime/RuntimeTypes.h"
#include "../runtime/objects/GameObject.h"
#include "../rendering/Renderer.h"
#include "../physics/physics.h"
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
      Anjean::Physics::Physics* physicsWorld;
      std::vector<Runtime::GameObject*> gameObjectsToRender;
      Runtime::Camera currentCamera;
      RendererState renderState;
      void GetRendererData();
      void GetRuntimeData();
      void PostToRuntime();
      void PostToRenderer();
      void HandleSDLEvent(const SDL_Event& event);
      void Tick();
      void PhysicsTick(float deltaTime);

    private:
        using Clock = std::chrono::steady_clock;

        Clock::time_point lastFrameTime;
        Clock::time_point lastFpsPrintTime;
        int framesSinceLastFpsPrint = 0;

        double physicsAccumulator = 0.0;

        static constexpr double FIXED_PHYSICS_DELTA = 1.0 / 60.0;

        static constexpr int MAX_PHYSICS_STEPS_PER_FRAME = 5;

  };
}