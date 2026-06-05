#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Orchestrator.hpp"

#include "../runtime/Runtime.h"
#include "../rendering/Renderer.h"
#include "../rendering/Utilities.hpp"
#include "../window/Window.h"
#include "../runtime/scripting/NativeBindings.h"

namespace Anjean::Orchestrator
{
  Orchestrator::Orchestrator() {
    runtime = new Anjean::Runtime::Runtime();
    renderState = RendererState();

    /** Temp */
    // Runtime::GameObject testGameObject{};
    // testGameObject.transform.position.x = 0;
    // testGameObject.transform.position.y = 0;
    // testGameObject.transform.position.z = -2;
    // testGameObject.transform.rotation.x = 0;
    // testGameObject.transform.rotation.y = 45;
    // testGameObject.transform.rotation.z = 0;
    // testGameObject.transform.scale.x = 1;
    // testGameObject.transform.scale.y = 1;
    // testGameObject.transform.scale.z = 1;
    // testGameObject.mesh = Runtime::Mesh();
    // testGameObject.mesh.value().id = runtime->getAllMeshes().at(0).id;
    // testGameObject.mesh.value().vertexCount = runtime->getAllMeshes().at(0).vertexCount;
    // testGameObject.mesh.value().vertices = runtime->getAllMeshes().at(0).vertices;
    // Runtime::Texture tempTexture{};
    // tempTexture.filename="text.png";
    // tempTexture.width=36;
    // tempTexture.height=36;
    // tempTexture.channels=4;
    // testGameObject.texture = tempTexture;

    // runtime->sceneObjects.emplace_back(std::move(testGameObject));
    // Runtime::GameObject testGameObject{};

  // runtime->sceneObjects.emplace_back(testGameObject);

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
    runtime->beginTick();
    runtime->executeTick();
    

    gameObjectsToRender = runtime->getRenderableSceneObjects();
    currentCamera = runtime->getCurrentCamera();

    if (renderer) {
      renderer->beginFrame({ 0.1f, 0.1f, 0.12f, 1.0f });
      for (Runtime::GameObject gO : gameObjectsToRender) {
        Rendering::ObjectUniformHandle objectUniformHandle;
        objectUniformHandle.modelTranslation = {
          simd_make_float4(1,0,0,0),
          simd_make_float4(0,1,0,0),
          simd_make_float4(0,0,1,0),
          simd_make_float4(gO.transform.position.x,gO.transform.position.y,gO.transform.position.z,1)
        };
        objectUniformHandle.modelRot = Rendering::makeRotationY(gO.transform.rotation.y);
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

        simd_float4x4 cameraMatrix = matrix_multiply(
          matrix_multiply({
              simd_make_float4(1,0,0,0),
              simd_make_float4(0,1,0,0),
              simd_make_float4(0,0,1,0),
              simd_make_float4(-currentCamera.transform.position.x,-currentCamera.transform.position.y,-currentCamera.transform.position.z,1)
            }, 
            Rendering::makeRotationY(0)
          ),
          {
            simd_make_float4(1,0,0,0),
            simd_make_float4(0,1,0,0),
            simd_make_float4(0,0,1,0),
            simd_make_float4(0,0,0,1)
          }
        );
        objectUniformHandle.viewProjection = matrix_multiply(projectionMatrix, cameraMatrix);
        auto meshIt = renderState.runtimeRendererMeshMap.find(gO.mesh.value().id);

        if (meshIt == renderState.runtimeRendererMeshMap.end()) {
            SDL_Log("Mesh id not found: %u", gO.mesh.value().id);
            continue;
        }

        const Core::MeshData& meshData = meshIt->second;

        std::optional<Rendering::TextureHandle> texture = std::nullopt;

        if (gO.texture.has_value()) {
          auto textureIt = renderState.runtimeTextureRendererTextureHandleMap.find(gO.texture.value().filename);
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
}