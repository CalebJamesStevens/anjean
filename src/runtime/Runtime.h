#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "RuntimeTypes.h"
#include "input/InputManager.h"
#include "objects/Camera.h"
#include "objects/GameObject.h"
#include "objects/PhysicsBody.h"
#include "scripting/ScriptingEngine.h"

namespace Anjean::Runtime {
  class Runtime {
  public:
    Runtime();

    void beginTick();
    void executeTick();
    void endTick();
    void executePhysicsTick(float deltaTime);

    GameObject& createGameObject();
    Camera& createCamera();

    GameObject& getGameObjectById(std::uint32_t id);

    void setCurrentCamera(std::uint32_t cameraId);
    Camera& getCurrentCamera();

    std::vector<GameObject*> getRenderableSceneObjects();
    std::vector<GameObject*> getPhysicsAwareSceneObjects();

    std::vector<Mesh> getAllMeshes();
    std::vector<Texture> getAllTextures();
    PhysicsBody& createPhysicsBody(Core::PhysicsBodyType type);
    PhysicsBody& getPhysicsBodyById(std::uint32_t physicsBodyId);

    void setGameObjectPhysicsBody(std::uint32_t gameObjectId, std::uint32_t physicsBodyId);

    std::uint32_t getGameObjectPhysicsBody(std::uint32_t gameObjectId);
    InputManager inputManager;
    Runtime* getRuntime();

  private:
    std::vector<std::unique_ptr<GameObject>> sceneObjects;
    std::vector<std::unique_ptr<PhysicsBody>> physicsBodies;
    std::uint32_t nextPhysicsBodyId = 1;
    std::uint32_t nextGameObjectId = 1;
    std::uint32_t currentCameraId = 0;

    ScriptingEngine scriptingEngine;
  };
} // namespace Anjean::Runtime