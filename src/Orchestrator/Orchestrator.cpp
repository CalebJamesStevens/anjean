#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Orchestrator.hpp"

#include "../physics/physics.h"
#include "../rendering/Renderer.h"
#include "../rendering/RendererSelection.h"
#include "../rendering/Utilities.hpp"
#include "../runtime/ModelLoader.h"
#include "../runtime/RenderSystem.h"
#include "../runtime/Runtime.h"
#include "../window/Window.h"
#include <ktx.h>

namespace Anjean::Orchestrator
{
std::vector<std::filesystem::path> getGltfFiles()
{
	namespace fs = std::filesystem;

	std::vector<fs::path> files;
	const fs::path        assetsDir = fs::current_path() / "Assets";

	if (!fs::exists(assetsDir) || !fs::is_directory(assetsDir))
	{
		return files;
	}

	for (const fs::directory_entry &entry : fs::recursive_directory_iterator(assetsDir))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const fs::path   &path = entry.path();
		const std::string ext  = path.extension().string();

		if (ext == ".gltf" || ext == ".glb")
		{
			files.push_back(path);
		}
	}

	return files;
}

void createTexturePacket(std::string path)
{
	ktxTexture    *kTexture;
	KTX_error_code result = ktxTexture_CreateFromNamedFile(
	    path.c_str(),
	    KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
	    &kTexture);

	if (result != KTX_SUCCESS)
	{
		throw std::runtime_error("failed to load ktx texture image!");
	}

	// Get texture dimensions and data
	uint32_t     texWidth       = kTexture->baseWidth;
	uint32_t     texHeight      = kTexture->baseHeight;
	ktx_size_t   imageSize      = ktxTexture_GetImageSize(kTexture, 0);
	ktx_uint8_t *ktxTextureData = ktxTexture_GetData(kTexture);
}

Orchestrator::Orchestrator()
{
	lastFrameTime    = Clock::now();
	lastFpsPrintTime = Clock::now();

	runtime      = Anjean::Runtime::Runtime::GetInstance();
	physicsWorld = new Anjean::Physics::Physics();
	renderState  = RendererState();
	Anjean::Runtime::MeshLoader modelLoader;

	try
	{
		Anjean::Rendering::RendererConfig rendererConfig{};
		rendererConfig.api = Anjean::Rendering::GraphicsAPI::Auto;

		rendererConfig.api =
		    Anjean::Rendering::ResolveGraphicsAPI(rendererConfig.api);

		const auto windowFlags =
		    Anjean::Rendering::GetSDLWindowFlagsForGraphicsAPI(rendererConfig.api);

		auto window =
		    new Anjean::Window("Anjean", 1280.0f, 880.0f, windowFlags);

		window->width  = 1280;
		window->height = 880;
		renderer       = new Anjean::Rendering::Renderer(*window);

		for (const auto &texture : runtime->getAllTextures())
		{
			decltype(Rendering::TextureHandle::id) returnTextureId;
			Rendering::TextureDesc                 texDesc{};
			texDesc.filename                                                     = texture.filename.c_str();
			texDesc.height                                                       = texture.height;
			texDesc.width                                                        = texture.width;
			texDesc.channels                                                     = texture.channels;
			Rendering::TextureHandle result                                      = renderer->createTexture(texDesc);
			renderState.runtimeTextureRendererTextureHandleMap[texDesc.filename] = result;
		}

		auto modelPaths = getGltfFiles();

		std::vector<const Core::ImportedModel *> importedModels{};

		for (const auto &modelPath : modelPaths)
		{
			Core::ModelAssetId modelId = modelLoader.loadModelAsset(modelPath);

			if (const Core::ImportedModel *model = modelLoader.getModel(modelId))
			{
				importedModels.push_back(model);
			}
		}

		renderer->loadModelToGPU(importedModels);

		auto playerE = runtime->GetCoordinator()->CreateEntity();

		runtime->GetCoordinator()->RegisterComponent<Core::Model>();

		runtime->GetCoordinator()->RegisterComponent<Anjean::Core::TransformComponent>();
		runtime->GetCoordinator()->RegisterComponent<Anjean::Core::RenderComponent>();

		renderSystem = runtime->GetCoordinator()->RegisterSystem<Runtime::RenderSystem>();

		Anjean::Runtime::Signature signature;
		signature.set(runtime->GetCoordinator()->GetComponentType<Anjean::Core::TransformComponent>());
		signature.set(runtime->GetCoordinator()->GetComponentType<Anjean::Core::RenderComponent>());

		runtime->GetCoordinator()->SetSystemSignature<Runtime::RenderSystem>(signature);

		runtime->GetCoordinator()->AddComponent(
		    playerE,
		    Anjean::Core::TransformComponent{
		        .position = {0.0f, 0.0f, -0.5f},
		        .rotation = {-20.0f, 0.0f, 0.0f},
		        .scale    = {1.0f, 1.0f, 1.0f},
		    });

		runtime->GetCoordinator()->AddComponent(
		    playerE,
		    Anjean::Core::RenderComponent{
		        .meshId = 1});
	}
	catch (const std::exception &e)
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

void Orchestrator::Orchestrator::HandleSDLEvent(const SDL_Event &event)
{
	runtime->inputManager.handleSDLEvent(event);
}

void Orchestrator::Tick()
{
	auto now = Clock::now();

	double frameDelta = std::chrono::duration<double>(now - lastFrameTime).count();

	framesSinceLastFpsPrint++;
	double fpsPrintDelta = std::chrono::duration<double>(now - lastFpsPrintTime).count();

	if (fpsPrintDelta >= 1.0)
	{
		double fps = framesSinceLastFpsPrint / fpsPrintDelta;

		std::cout << "FPS: " << fps << std::endl;

		framesSinceLastFpsPrint = 0;
		lastFpsPrintTime        = now;
	}

	lastFrameTime = now;

	if (frameDelta > 0.25)
	{
		frameDelta = 0.25;
	}

	physicsAccumulator += frameDelta;

	runtime->beginTick();

	int physicsSteps = 0;

	while (physicsAccumulator >= FIXED_PHYSICS_DELTA &&
	       physicsSteps < MAX_PHYSICS_STEPS_PER_FRAME)
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
	currentCamera       = runtime->getCurrentCamera();

	if (renderer)
	{
		renderer->beginFrame({0.1f, 0.1f, 0.12f, 1.0f});

		// std::vector<Anjean::Core::RenderPacket> renderPackets{};

		// for (auto *mesh : runtime->GetCoordinator()->GetComponent<Core::Model>(runtime->GetCoordinator().))
		// {
		// 	if (!gO->mesh.has_value() || gO->mesh.value() == nullptr)
		// 	{
		// 		continue;
		// 	}

		// 	Anjean::Core::RenderPacket packet{
		// 	    .objectId = gO->id,
		// 	    .meshId   = gO->mesh.value()->id,
		// 	    .position = {gO->transform.position.x, gO->transform.position.y, gO->transform.position.z},
		// 	    .rotation = {
		// 	        glm::radians(gO->transform.rotation.x),
		// 	        glm::radians(gO->transform.rotation.y),
		// 	        glm::radians(gO->transform.rotation.z)},
		// 	    .scale = {gO->transform.scale.x, gO->transform.scale.y, gO->transform.scale.z}};

		// 	renderPackets.emplace_back(packet);
		// }

		Anjean::Core::CameraPacket cameraPacket{
		    .aspectRatio = currentCamera.aspectRatio,
		    .farPlane    = currentCamera.farClippingPlane,
		    .fieldOfView = currentCamera.fieldOfView,
		    .nearPlane   = currentCamera.nearClippingPlane,
		    .objectId    = currentCamera.id,
		    .position    = {currentCamera.transform.position.x, currentCamera.transform.position.y, currentCamera.transform.position.z},
		    .rotation    = {
		        glm::radians(currentCamera.transform.rotation.x),
		        glm::radians(currentCamera.transform.rotation.y),
		        glm::radians(currentCamera.transform.rotation.z)}};

		auto renderPackets = renderSystem->buildRenderPackets(*runtime->GetCoordinator());
		renderer->renderFrame(cameraPacket, {.a = 0, .b = 0, .r = 0, .g = 0}, renderPackets);

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

	std::unordered_map<std::uint32_t, Runtime::GameObject *> bodyIdToObject;

	for (auto *obj : objectsToSimulate)
	{
		if (!obj->physicsBodyId.has_value())
		{
			continue;
		}

		std::uint32_t physicsBodyId = obj->physicsBodyId.value();

		auto &body = runtime->getPhysicsBodyById(physicsBodyId);

		Core::PhysicsBodyState state{};

		state.id = physicsBodyId;

		// For now, the GameObject transform is the body's world position.
		state.position = obj->transform.position;

		state.velocity         = body.velocity;
		state.force            = body.force;
		state.mass             = body.mass;
		state.physicsBodyType  = body.getPhysicsBodyType();
		state.kinematicMove    = body.pendingKinematicMove;
		state.hasKinematicMove = body.hasPendingKinematicMove;
		if (state.physicsBodyType == Core::PhysicsBodyType::Kinematic)
		{
			state.mass = INFINITY;
		}
		for (auto bodyCollider : body.colliders)
		{
			Core::ColliderState colState{};
			colState.id           = bodyCollider.id;
			colState.localCenter  = bodyCollider.localCenter;
			colState.radius       = bodyCollider.radius;
			colState.halfExtents  = bodyCollider.halfExtents;
			colState.colliderType = bodyCollider.type;
			state.colliders.emplace_back(colState);
		}

		physicsWorldBodies.emplace_back(state);

		// Keep explicit link so we can sync the transform after simulation.
		bodyIdToObject[physicsBodyId] = obj;
	}

	auto simulatedResults = physicsWorld->Step(deltaTime, physicsWorldBodies);

	for (const Core::PhysicsBodyResult &result : simulatedResults)
	{
		auto &body = runtime->getPhysicsBodyById(result.id);

		body.velocity                = result.velocity;
		body.pendingKinematicMove    = Core::Vector3{0, 0, 0};
		body.hasPendingKinematicMove = false;

		auto it = bodyIdToObject.find(result.id);

		if (it == bodyIdToObject.end())
		{
			continue;
		}

		Runtime::GameObject *obj = it->second;

		obj->transform.position = result.position;
	}
}
}        // namespace Anjean::Orchestrator