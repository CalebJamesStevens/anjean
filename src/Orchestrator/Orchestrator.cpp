#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include "Orchestrator.hpp"

#include "../runtime/Runtime.h"
#include "../rendering/Renderer.h"
#include "../rendering/Utilities.hpp"
#include "../window/Window.h"
#include "../runtime/scripting/NativeBindings.h"
#include "../physics/physics.h"

namespace Anjean::Orchestrator
{
  Orchestrator::Orchestrator() {
    lastFrameTime = Clock::now();
    lastFpsPrintTime = Clock::now();

    runtime = new Anjean::Runtime::Runtime();
    physicsWorld = new Anjean::Physics::Physics();
    renderState = RendererState();

    try
    {
        auto window = new Anjean::Window(
            "Anjean",
            1280.0f,
            880.0f,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_METAL
        );

        renderer = new Anjean::Rendering::Renderer(*window);
        
        renderState.spritePipelineHandle = renderer->createSpritePipeline();
        for (const auto& rtMesh : runtime->getAllMeshes()) {
          Rendering::Mesh rendererMesh;
          rendererMesh.vertices = rtMesh.vertices;
          rendererMesh.vertexCount = rtMesh.vertexCount;

          std::pair<decltype(Rendering::BufferHandle::id), std::optional<decltype(Rendering::TextureHandle::id)>> result = renderer->loadMeshToGPU(rendererMesh);
          Core::MeshData meshData;
          meshData.id = result.first;
          meshData.vertices.resize(rtMesh.vertices.size());

          // Copy element by element without using operator= on the vector itself
          std::copy(rtMesh.vertices.begin(), rtMesh.vertices.end(), meshData.vertices.begin());
          
          renderState.runtimeRendererMeshMap[rtMesh.id]= meshData;
        }
        for (const auto& texture : runtime->getAllTextures()) {
        
          decltype(Rendering::TextureHandle::id) returnTextureId;
          Rendering::TextureDesc texDesc{};
          texDesc.filename = texture.filename.c_str();
          texDesc.height = texture.height;
          texDesc.width = texture.width;
          texDesc.channels = texture.channels;
          Rendering::TextureHandle result = renderer->createTexture(texDesc);
          renderState.runtimeTextureRendererTextureHandleMap[texDesc.filename] = result;
        }
      }
    catch (const std::exception& e)
    {
        SDL_Log("Startup failed: %s", e.what());
    }

  };

  void Orchestrator::GetRendererData() {

  };
  void Orchestrator::GetRuntimeData() {

  };
  void Orchestrator::PostToRuntime() {

  };
  void Orchestrator::PostToRenderer() {

  };

  void Orchestrator::Orchestrator::HandleSDLEvent(const SDL_Event& event)
  {
      runtime->inputManager.handleSDLEvent(event);
  }

  void Orchestrator::Tick() {
    auto now = Clock::now();

    double frameDelta = std::chrono::duration<double>(
        now - lastFrameTime
    ).count();

    framesSinceLastFpsPrint++;
    double fpsPrintDelta = std::chrono::duration<double>(
        now - lastFpsPrintTime
    ).count();

    if (fpsPrintDelta >= 1.0)
    {
        double fps = framesSinceLastFpsPrint / fpsPrintDelta;

        std::cout << "FPS: " << fps << std::endl;

        framesSinceLastFpsPrint = 0;
        lastFpsPrintTime = now;
    }

    lastFrameTime = now;

    if (frameDelta > 0.25)
    {
        frameDelta = 0.25;
    }

    physicsAccumulator += frameDelta;

    runtime->beginTick();

    int physicsSteps = 0;

    while (
        physicsAccumulator >= FIXED_PHYSICS_DELTA &&
        physicsSteps < MAX_PHYSICS_STEPS_PER_FRAME
    )
    {
        PhysicsTick(static_cast<float>(FIXED_PHYSICS_DELTA));

        physicsAccumulator -= FIXED_PHYSICS_DELTA;
        physicsSteps++;
    }

    // If we hit the safety cap, drop leftover time
    if (physicsSteps == MAX_PHYSICS_STEPS_PER_FRAME)
    {
        physicsAccumulator = 0.0;
    }

    runtime->executeTick();
    

    gameObjectsToRender = runtime->getRenderableSceneObjects();
    currentCamera = runtime->getCurrentCamera();

    if (renderer) {
      renderer->beginFrame({ 0.1f, 0.1f, 0.12f, 1.0f });
      for (Runtime::GameObject* gO : gameObjectsToRender) {
        Rendering::ObjectUniformHandle objectUniformHandle;
        objectUniformHandle.modelTranslation = {
          simd_make_float4(1,0,0,0),
          simd_make_float4(0,1,0,0),
          simd_make_float4(0,0,1,0),
          simd_make_float4(gO->transform.position.x,gO->transform.position.y,gO->transform.position.z,1)
        };
        objectUniformHandle.modelRot = matrix_multiply(
          Rendering::makeRotationZ(gO->transform.rotation.z* (3.14159265358979323846f / 180.0f)),
          matrix_multiply(
            Rendering::makeRotationY(gO->transform.rotation.y* (3.14159265358979323846f / 180.0f)),
            Rendering::makeRotationX(gO->transform.rotation.x* (3.14159265358979323846f / 180.0f)))
        );

        objectUniformHandle.modelScale = {
          simd_make_float4(1,0,0,0),
          simd_make_float4(0,1,0,0),
          simd_make_float4(0,0,1,0),
          simd_make_float4(0,0,0,1)
        };
        objectUniformHandle.model = matrix_multiply(
            matrix_multiply(objectUniformHandle.modelTranslation, objectUniformHandle.modelRot),
            objectUniformHandle.modelScale
        );
        simd_float4x4 projectionMatrix = Rendering::makePerspective(
            70.0f * M_PI / 180.0f,
            1280.0f/880.0f,
            0.1f,
            100.0f
        );

        auto cameraRot = matrix_multiply(Rendering::makeRotationZ(currentCamera.transform.rotation.z* (3.14159265358979323846f / 180.0f)), 
        matrix_multiply(Rendering::makeRotationY(currentCamera.transform.rotation.y* (3.14159265358979323846f / 180.0f)),
          Rendering::makeRotationX(currentCamera.transform.rotation.x* (3.14159265358979323846f / 180.0f))));
        simd_float4x4 cameraMatrix = matrix_multiply(
          matrix_multiply({
              simd_make_float4(1,0,0,0),
              simd_make_float4(0,1,0,0),
              simd_make_float4(0,0,1,0),
              simd_make_float4(-currentCamera.transform.position.x,-currentCamera.transform.position.y,-currentCamera.transform.position.z,1)
            }, 
            cameraRot
          ),
          {
            simd_make_float4(1,0,0,0),
            simd_make_float4(0,1,0,0),
            simd_make_float4(0,0,1,0),
            simd_make_float4(0,0,0,1)
          }
        );
        objectUniformHandle.viewProjection = matrix_multiply(projectionMatrix, cameraMatrix);
        auto meshIt = renderState.runtimeRendererMeshMap.find(gO->mesh.value().id);

        if (meshIt == renderState.runtimeRendererMeshMap.end()) {
            SDL_Log("Mesh id not found: %u", gO->mesh.value().id);
            continue;
        }

        const Core::MeshData& meshData = meshIt->second;

        std::optional<Rendering::TextureHandle> texture = std::nullopt;

        if (gO->texture.has_value()) {
          auto textureIt = renderState.runtimeTextureRendererTextureHandleMap.find(gO->texture.value().filename);
          if (textureIt != renderState.runtimeTextureRendererTextureHandleMap.end()) {
            texture = textureIt->second;
          } 
        } 

        renderer->drawSprite(
            renderState.spritePipelineHandle,
            meshData,
            texture,
            objectUniformHandle
        );
      }
      renderer->endFrame();
    }
    runtime->endTick();

  };

  void Orchestrator::PhysicsTick(float deltaTime)
  {
      // Later:
     runtime->executePhysicsTick(deltaTime);

    auto objectsToSimulate = runtime->getPhysicsAwareSceneObjects();

    std::vector<Core::PhysicsBodyState> physicsWorldBodies;
    physicsWorldBodies.reserve(objectsToSimulate.size());

    std::unordered_map<std::uint32_t, Runtime::GameObject*> bodyIdToObject;

    for (auto* obj : objectsToSimulate)
    {
        if (!obj->physicsBodyId.has_value())
        {
            continue;
        }

        std::uint32_t physicsBodyId = obj->physicsBodyId.value();

        auto& body = runtime->getPhysicsBodyById(physicsBodyId);

        Core::PhysicsBodyState state{};

        state.id = physicsBodyId;

        // For now, the GameObject transform is the body's world position.
        state.position = obj->transform.position;

        state.velocity = body.velocity;
        state.force = body.force;
        state.mass = body.mass;
        state.physicsBodyType = body.getPhysicsBodyType();
        
        for (auto bodyCollider : body.colliders) {
          Core::ColliderState colState{};
          colState.id = bodyCollider.id;
          colState.localCenter = bodyCollider.localCenter;
          colState.radius = bodyCollider.radius;
          colState.halfExtents = bodyCollider.halfExtents;
          colState.colliderType = bodyCollider.type;
          state.colliders.emplace_back(colState);
        }

        physicsWorldBodies.emplace_back(state);

        // Keep explicit link so we can sync the transform after simulation.
        bodyIdToObject[physicsBodyId] = obj;
    }

    auto simulatedResults = physicsWorld->Step(deltaTime, physicsWorldBodies);

    for (const Core::PhysicsBodyResult& result : simulatedResults)
    {
        auto& body = runtime->getPhysicsBodyById(result.id);

        body.velocity = result.velocity;

        auto it = bodyIdToObject.find(result.id);

        if (it == bodyIdToObject.end())
        {
            continue;
        }

        Runtime::GameObject* obj = it->second;

        obj->transform.position = result.position;
    }
  }
}