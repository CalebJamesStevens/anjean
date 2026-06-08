#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <stdexcept>

#include "objects/GameObject.h"
#include "objects/Camera.h"
#include "RuntimeTypes.h"
#include "scripting/ScriptingEngine.h"
#include "input/InputManager.h"
#include "objects/PhysicsBody.h"

namespace Anjean::Runtime
{
    class Runtime
    {
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

        void setGameObjectPhysicsBody(
            std::uint32_t gameObjectId,
            std::uint32_t physicsBodyId
        );

        std::uint32_t getGameObjectPhysicsBody(
            std::uint32_t gameObjectId
        );
        InputManager inputManager;

    private:
        std::vector<std::unique_ptr<GameObject>> sceneObjects;
        std::vector<std::unique_ptr<PhysicsBody>> physicsBodies;
        std::uint32_t nextPhysicsBodyId = 1;
        std::uint32_t nextGameObjectId = 1;
        std::uint32_t currentCameraId = 0;

        ScriptingEngine scriptingEngine;
    };
}