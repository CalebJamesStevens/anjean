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
#include "../runtime/Runtime.h"
#include "../runtime/scripting/NativeBindings.h"
#include "../window/Window.h"

namespace Anjean::Orchestrator
{
Orchestrator::Orchestrator()
{
	lastFrameTime    = Clock::now();
	lastFpsPrintTime = Clock::now();

	runtime      = Anjean::Runtime::Runtime::GetInstance();
	physicsWorld = new Anjean::Physics::Physics();
	renderState  = RendererState();

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

		renderState.spritePipelineHandle = renderer->createSpritePipeline();
		for (const auto &rtMesh : runtime->getAllMeshes())
		{
			Rendering::Mesh rendererMesh;
			rendererMesh.vertices    = rtMesh->vertices;
			rendererMesh.vertexCount = rtMesh->vertexCount;

			std::pair<decltype(Rendering::BufferHandle::id),
			          std::optional<decltype(Rendering::TextureHandle::id)>>
			               result = renderer->loadMeshToGPU({.id       = rtMesh->id,
			                                                 .indices  = {},
			                                                 .vertices = rtMesh->vertices});
			Core::MeshData meshData;
			meshData.id = result.first;
			meshData.vertices.resize(rtMesh->vertices.size());

			// Copy element by element without using operator= on the vector itself
			std::copy(rtMesh->vertices.begin(), rtMesh->vertices.end(), meshData.vertices.begin());

			renderState.runtimeRendererMeshMap[rtMesh->id] = meshData;
		}
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
		simd_float4x4 projectionMatrix =
		    Rendering::makePerspective(70.0f * M_PI / 180.0f, 1280.0f / 880.0f, 0.1f, 100.0f);

		std::vector<Anjean::Core::RenderPacket> renderPackets{};

		for (Runtime::GameObject *gO : gameObjectsToRender)
		{
			if (!gO->mesh.has_value() || gO->mesh.value() == nullptr)
			{
				continue;
			}

			auto meshIt = renderState.runtimeRendererMeshMap.find(gO->mesh.value()->id);

			if (meshIt == renderState.runtimeRendererMeshMap.end())
			{
				SDL_Log("Mesh id not found: %u", gO->mesh.value()->id);
				continue;
			}

			const Core::MeshData &meshData = meshIt->second;

			std::optional<Rendering::TextureHandle> texture = std::nullopt;

			if (gO->texture.has_value())
			{
				auto textureIt =
				    renderState.runtimeTextureRendererTextureHandleMap.find(gO->texture.value().filename);
				if (textureIt != renderState.runtimeTextureRendererTextureHandleMap.end())
				{
					texture = textureIt->second;
				}
			}

			Anjean::Core::RenderPacket packet{
			    .objectId = gO->id,
			    .meshId   = gO->mesh.value()->id,
			    .position = {gO->transform.position.x, gO->transform.position.y, gO->transform.position.z},
			    .rotation = {
			        glm::radians(gO->transform.rotation.x),
			        glm::radians(gO->transform.rotation.y),
			        glm::radians(gO->transform.rotation.z)},
			    .scale = {gO->transform.scale.x, gO->transform.scale.y, gO->transform.scale.z}};

			renderPackets.emplace_back(packet);
		}

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

		std::ranges::sort(renderPackets, {}, &Anjean::Core::RenderPacket::objectId);

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