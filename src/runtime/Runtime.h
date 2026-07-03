// hi
// -Adam

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "EntityManager.h"
#include "RuntimeTypes.h"
#include "input/InputManager.h"
#include "objects/Camera.h"
#include "objects/GameObject.h"
#include "objects/NativeMeshes/Mesh.h"
#include "objects/PhysicsBody.h"

namespace Anjean::Runtime
{
class Runtime
{
  public:
	void beginTick();
	void executeTick();
	void endTick();
	void executePhysicsTick(float deltaTime);

	GameObject &createGameObject();
	Mesh       &createMesh(Anjean::Core::MeshDescriptor meshDescriptor);
	Camera     &createCamera();

	GameObject &getGameObjectById(std::uint32_t id);

	void    setCurrentCamera(std::uint32_t cameraId);
	Camera &getCurrentCamera();

	std::vector<GameObject *> getRenderableSceneObjects();
	std::vector<GameObject *> getPhysicsAwareSceneObjects();

	std::vector<Mesh *>       getAllMeshes();
	std::vector<const Mesh *> getAllMeshes() const;
	std::vector<Texture>      getAllTextures();
	PhysicsBody              &createPhysicsBody(Core::PhysicsBodyType type);
	PhysicsBody              &getPhysicsBodyById(std::uint32_t physicsBodyId);

	void setGameObjectPhysicsBody(std::uint32_t gameObjectId, std::uint32_t physicsBodyId);

	std::uint32_t getGameObjectPhysicsBody(std::uint32_t gameObjectId);
	InputManager  inputManager;
	Runtime      *getRuntime();
	Runtime(Runtime &other)                        = delete;
	void                operator=(const Runtime &) = delete;
	static Runtime     *GetInstance();
	static Coordinator *GetCoordinator();

  private:
	Runtime()
	{}
	static Runtime                           *runtime_;
	std::vector<std::unique_ptr<GameObject>>  sceneObjects;
	std::vector<std::unique_ptr<Mesh>>        meshes;
	std::vector<std::unique_ptr<PhysicsBody>> physicsBodies;
	std::uint32_t                             nextPhysicsBodyId   = 1;
	std::uint32_t                             nextGameObjectId    = 1;
	std::uint32_t                             nextMeshId          = 1;
	std::uint32_t                             nextRuntimeObjectId = 1;
	std::uint32_t                             currentCameraId     = 0;
	static Coordinator                       *gCoordinator;
};
}        // namespace Anjean::Runtime
