#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <chrono>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <ktx.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <vulkan/vulkan.hpp>

#include "../../../runtime/RuntimeTypes.h"
#include "../../../window/Window.h"
#include "VulkanRendererBackend.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// binding 0 = FrameDataUBO
//  binding 1 = ObjectData SSBO
//  binding 2 = MaterialData SSBO
//  binding 3 = GeometryData SSBO
//  binding 4 = DrawData SSBO
//  binding 5 = textures[]
//  binding 6 = samplers[]

struct PushConstants
{
	uint32_t passIndex;
	uint32_t drawIndex;
};

// Vertex structure with position, normal, color, and texture coordinates
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 texCoord;

	// Binding and attribute descriptions for Vulkan
	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
	}

	static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
	{
		return {
		    vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
		    vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
		    vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
		    vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))};
	}

	// Equality operator and hash function for vertex deduplication
	bool operator==(const Vertex &other) const
	{
		return pos == other.pos && normal == other.normal && color == other.color && texCoord == other.texCoord;
	}
};

// Structure for PBR material properties
struct Material
{
	glm::vec4 baseColorFactor = glm::vec4(1.0f);
	float     metallicFactor  = 1.0f;
	float     roughnessFactor = 1.0f;
	glm::vec3 emissiveFactor  = glm::vec3(0.0f);

	int baseColorTextureIndex         = -1;
	int metallicRoughnessTextureIndex = -1;
	int normalTextureIndex            = -1;
	int occlusionTextureIndex         = -1;
	int emissiveTextureIndex          = -1;
};

struct MaterialData
{
	glm::vec4 baseColorFactor;
	float     metallicFactor;
	float     roughnessFactor;
	int       baseColorTexture;
	int       normalTexture;
	int       metallicRoughnessTexture;
	int       emissiveTexture;
};

// Structure for a mesh with vertices, indices, and material
struct Mesh
{
	std::vector<Vertex>   vertices;
	std::vector<uint32_t> indices;
	int                   materialIndex = -1;
};

namespace std
{
template <>
struct hash<Vertex>
{
	size_t operator()(Vertex const &vertex) const
	{
		return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
		       (hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};
}        // namespace std

namespace Anjean::Rendering
{
constexpr uint32_t WIDTH                = 800;
constexpr uint32_t HEIGHT               = 600;
const std::string  MODEL_PATH           = "models/viking_room.gltf";
const std::string  TEXTURE_PATH         = "/Users/caleb/repos/anjean/2d_rgba8.ktx2";
constexpr int      MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<char const *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct ObjectData
{
	glm::mat4 modelMatrix;
};

struct GpuMesh
{
	Anjean::Core::MeshId id = 0;

	vk::raii::Buffer       vertexBuffer       = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;

	vk::raii::Buffer       indexBuffer       = nullptr;
	vk::raii::DeviceMemory indexBufferMemory = nullptr;

	uint32_t indexCount  = 0;
	uint32_t vertexCount = 0;
};

// Define a structure to hold per-object data
struct RenderObject
{
	Anjean::Core::ObjectId objectId = 0;
	Anjean::Core::MeshId   meshId   = 0;

	// Transform properties
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
	glm::vec3 scale    = {1.0f, 1.0f, 1.0f};

	// Descriptor sets for this object (one per frame in flight)
	std::vector<vk::raii::DescriptorSet> descriptorSets;

	// Calculate model matrix based on position, rotation, and scale
	glm::mat4 getModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model           = glm::translate(model, position);
		model           = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model           = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model           = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model           = glm::scale(model, scale);
		return model;
	}
};

// bool cameraInside = (camPos.x >= wmin.x && camPos.x <= wmax.x &&
// camera->ForceViewMatrixUpdate();
// CameraComponent* camera,
// const bool doCulling = enableFrustumCulling && camera;
// const glm::mat4 vp = proj * camera->GetViewMatrix();
// float farZ = camera->GetFarPlane();
// float fov = glm::radians(camera->GetFieldOfView());
// float nearZ = camera->GetNearPlane();
// frameUboTemplate.camPos = glm::vec4(camera->GetPosition(), 1.0f);
// frameUboTemplate.farZ = camera->GetFarPlane();
// frameUboTemplate.nearZ = camera->GetNearPlane();
// frameUboTemplate.proj = camera->GetProjectionMatrix();
// frameUboTemplate.view = camera->GetViewMatrix();
// glm::mat4 camView = camera->GetViewMatrix();
// glm::mat4 proj = camera->GetProjectionMatrix();
// glm::mat4 projReflected = camera ? camera->GetProjectionMatrix() : glm::mat4(1.0f);
// glm::mat4 view = camera->GetViewMatrix();
// glm::mat4 viewReflected = camera ? (camera->GetViewMatrix() * reflectM) : reflectM;
// glm::vec3 camPos = camera ? camera->GetPosition() : glm::vec3(0.0f);
// glm::vec3 camPos = camera->GetPosition();
// if (!camera) return;
// if (!cameraInside) {
// if (doCulling && camera) {
// if (enableDistanceLOD && camera) {
// prepareFrameUboTemplate(camera);
// refitTopLevelAS(entities, camera);
// Render(snapshot, camera, imguiSystem);
// renderReflectionPass(commandBuffers[currentFrame], planeWS, camera, opaqueJobs);
// ubo.camPos = glm::vec4(camera ? camera->GetPosition() : glm::vec3(0), 1.0f);
// ubo.camPos = glm::vec4(camera->GetPosition(), 1.0f);
// ubo.proj = camera->GetProjectionMatrix();
// ubo.view = camera->GetViewMatrix();
// updateLightStorageBuffer(currentFrame, lightsSubset, camera);
// updateUniformBuffer(currentFrame, entity, &entityRes, camera, tc);
// updateUniformBufferInternal(currentFrame, entity, entityRes, camera, ubo);
// updateUniformBufferInternal(currentImage, entity, entityRes, camera, ubo);
// void Renderer::prepareFrameUboTemplate(CameraComponent* camera) {
// void Renderer::Render(const std::vector<Entity *>& entities, CameraComponent* camera, ImGuiSystem* imguiSystem) {
// void Renderer::Render(const std::vector<std::unique_ptr<Entity>>& entities, CameraComponent* camera, ImGuiSystem* imguiSystem) {
// void Renderer::updateUniformBuffer(uint32_t currentImage, Entity* entity, EntityResources* entityRes, CameraComponent* camera, const glm::mat4& customTransform) {
// void Renderer::updateUniformBuffer(uint32_t currentImage, Entity* entity, EntityResources* entityRes, CameraComponent* camera, TransformComponent* tc) {
// void Renderer::updateUniformBufferInternal(uint32_t currentImage, Entity* entity, EntityResources* entityRes, CameraComponent* camera, UniformBufferObject& ubo) {

struct CameraObject
{
	// Anjean::Core::ObjectId objectId = 0;

	// Transform properties
	// glm::vec3 position = {0.0f, 0.0f, 0.0f};
	// glm::vec3 rotation = {0.0f, 0.0f, 0.0f};

	glm::mat4 viewMatrix       = glm::identity<glm::mat4>();
	glm::mat4 projectionMatrix = glm::identity<glm::mat4>();
};

struct FrameDataUBO
{
	CameraObject camera;
	float        time;
	uint32_t     frameIndex;
	glm::vec2    renderSize;
};

struct VulkanRendererBackend::Impl
{
	vk::raii::Context                                 context;
	vk::raii::Instance                                instance       = nullptr;
	vk::raii::PhysicalDevice                          physicalDevice = nullptr;
	vk::raii::Device                                  device         = nullptr;
	vk::PhysicalDeviceFeatures                        deviceFeatures;
	uint32_t                                          graphicsQueueIndex = ~0;
	vk::raii::Queue                                   graphicsQueue      = nullptr;
	vk::raii::SurfaceKHR                              surface            = nullptr;
	vk::ClearValue                                    clearColor         = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
	vk::raii::SwapchainKHR                            swapChain          = nullptr;
	std::vector<vk::Image>                            swapChainImages;
	vk::SurfaceFormatKHR                              swapChainSurfaceFormat;
	vk::Extent2D                                      swapChainExtent;
	std::vector<vk::raii::ImageView>                  swapChainImageViews;
	vk::raii::DescriptorSetLayout                     descriptorSetLayout = nullptr;
	vk::raii::PipelineLayout                          pipelineLayout      = nullptr;
	vk::raii::Pipeline                                graphicsPipeline    = nullptr;
	vk::raii::CommandPool                             commandPool         = nullptr;
	std::vector<vk::raii::CommandBuffer>              commandBuffers;
	std::vector<vk::raii::Semaphore>                  presentCompleteSemaphores;
	std::vector<vk::raii::Semaphore>                  renderFinishedSemaphores;
	std::vector<vk::raii::Fence>                      inFlightFences;
	uint32_t                                          frameIndex         = 0;
	bool                                              framebufferResized = false;
	vk::raii::DescriptorPool                          descriptorPool     = nullptr;
	vk::raii::Image                                   textureImage       = nullptr;
	vk::raii::DeviceMemory                            textureImageMemory = nullptr;
	vk::PipelineStageFlags                            sourceStage;
	vk::PipelineStageFlags                            destinationStage;
	vk::raii::ImageView                               textureImageView = nullptr;
	vk::raii::Sampler                                 textureSampler   = nullptr;
	vk::raii::Image                                   depthImage       = nullptr;
	vk::raii::DeviceMemory                            depthImageMemory = nullptr;
	vk::raii::ImageView                               depthImageView   = nullptr;
	std::unordered_map<Anjean::Core::MeshId, GpuMesh> gpuMeshes;
	std::vector<RenderObject>                         renderObjects;
	Anjean::Window                                   &window;
	CameraObject                                      activeCamera;
	// std::vector<GpuTexture>                           textures;
	std::vector<vk::raii::Sampler>      samplers;
	std::vector<vk::raii::Buffer>       frameDataUBOs;
	std::vector<vk::raii::DeviceMemory> frameDataUBOMemories;
	std::vector<void *>                 frameDataUBOMapped{};
	std::vector<vk::raii::Buffer>       objectDataSSBO;
	std::vector<vk::raii::DeviceMemory> objectDataSSBOMemories;
	std::vector<void *>                 objectDataSSBOMapped{};

	Impl(Anjean::Window &windowRef) : window(windowRef)
	{}

	static std::vector<char>
	    readFile(const char *path)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file)
		{
			throw std::runtime_error(std::string("Failed to open file: ") + path);
		}

		size_t            size = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(size);

		file.seekg(0);
		file.read(buffer.data(), size);
		file.close();

		return buffer;
	}

	std::vector<const char *> getRequiredInstanceExtensions()
	{
		Uint32             sdlExtensionCount = 0;
		char const *const *sdlExtensions     = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

		if (!sdlExtensions)
		{
			throw std::runtime_error(SDL_GetError());
		}

		std::vector<const char *> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

#ifdef __APPLE__
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

		return extensions;
	}

	std::vector<const char *> requiredDeviceExtension = {
	    vk::KHRSwapchainExtensionName,
#ifdef __APPLE__
	    "VK_KHR_portability_subset",
#endif
	};

	bool isDeviceSuitable(vk::raii::PhysicalDevice const &physicalDevice)
	{
		bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

		// Check if any of the queue families support graphics operations
		auto queueFamilies    = physicalDevice.getQueueFamilyProperties();
		bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const &qfp) {
			return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
		});

		// Check if all required physicalDevice extensions are available
		auto availableDeviceExtensions     = physicalDevice.enumerateDeviceExtensionProperties();
		bool supportsAllRequiredExtensions = std::ranges::all_of(
		    requiredDeviceExtension, [&availableDeviceExtensions](auto const &requiredDeviceExtension) {
			    return std::ranges::any_of(availableDeviceExtensions,
			                               [requiredDeviceExtension](auto const &availableDeviceExtension) {
				                               return strcmp(availableDeviceExtension.extensionName,
				                                             requiredDeviceExtension) == 0;
			                               });
		    });

		// Check if the physicalDevice supports the required features (shader draw parameters, dynamic
		// rendering and extended dynamic state)

		auto features = physicalDevice.template getFeatures2<
		    vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
		    vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

		auto const &v13 = features.get<vk::PhysicalDeviceVulkan13Features>();

		bool supportsRequiredFeatures =
		    features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
		    features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
		    v13.dynamicRendering && v13.synchronization2 &&
		    features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
		        .extendedDynamicState;

		std::cout << "supportsVulkan1_3: " << supportsVulkan1_3 << "\n";
		std::cout << "supportsGraphics: " << supportsGraphics << "\n";
		std::cout << "supportsAllRequiredExtensions: " << supportsAllRequiredExtensions << "\n";
		std::cout << "supportsRequiredFeatures: " << supportsRequiredFeatures << "\n";

		// Return true if the physicalDevice meets all the criteria
		return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions &&
		       supportsRequiredFeatures;
	}

	void createInstance()
	{
		constexpr vk::ApplicationInfo appInfo{.pApplicationName   = "Hello Triangle",
		                                      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		                                      .pEngineName        = "No Engine",
		                                      .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
		                                      .apiVersion         = vk::ApiVersion14};

		std::vector<const char *> requiredLayers;
		if (enableValidationLayers)
		{
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());
		}

		auto layerProperties = context.enumerateInstanceLayerProperties();

		for (const char *requiredLayer : requiredLayers)
		{
			bool found = std::ranges::any_of(layerProperties, [requiredLayer](const auto &layerProperty) {
				return strcmp(layerProperty.layerName, requiredLayer) == 0;
			});

			if (!found)
			{
				throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));
			}
		}

		auto requiredExtensions  = getRequiredInstanceExtensions();
		auto extensionProperties = context.enumerateInstanceExtensionProperties();

		for (const char *requiredExtension : requiredExtensions)
		{
			bool found = std::ranges::any_of(
			    extensionProperties, [requiredExtension](const auto &extensionProperty) {
				    return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
			    });

			if (!found)
			{
				throw std::runtime_error("Required extension not supported: " +
				                         std::string(requiredExtension));
			}
		}

		vk::InstanceCreateFlags flags{};

#ifdef __APPLE__
		flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

		vk::InstanceCreateInfo createInfo{
		    .flags                   = flags,
		    .pApplicationInfo        = &appInfo,
		    .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
		    .ppEnabledLayerNames     = requiredLayers.data(),
		    .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
		    .ppEnabledExtensionNames = requiredExtensions.data()};

		instance = vk::raii::Instance(context, createInfo);
	}

	void pickPhysicalDevice()
	{
		std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		auto const                            devIter         = std::ranges::find_if(physicalDevices, [&](auto const &physicalDevice) {
			return isDeviceSuitable(physicalDevice);
		});
		if (devIter == physicalDevices.end())
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
		physicalDevice = *devIter;
	}

	vk::SurfaceFormatKHR
	    chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
	{
		const auto formatIt = std::ranges::find_if(availableFormats, [](const auto &format) {
			return format.format == vk::Format::eB8G8R8A8Srgb &&
			       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
		return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
	}

	vk::PresentModeKHR
	    chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes)
	{
		assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {
			return presentMode == vk::PresentModeKHR::eFifo;
		}));
		return std::ranges::any_of(
		           availablePresentModes,
		           [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
		           vk::PresentModeKHR::eMailbox :
		           vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		int width, height;
		SDL_GetWindowSizeInPixels(window.getNativeWindow(), &width, &height);

		return {std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
		                             capabilities.maxImageExtent.width),
		        std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
		                             capabilities.maxImageExtent.height)};
	}

	uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities)
	{
		auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
		if ((0 < surfaceCapabilities.maxImageCount) &&
		    (surfaceCapabilities.maxImageCount < minImageCount))
		{
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> &code)
	{
		vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size() * sizeof(char),
		                                      .pCode    = reinterpret_cast<const uint32_t *>(code.data())};
		vk::raii::ShaderModule     shaderModule{device, createInfo};
		return shaderModule;
	}

	void createSwapChain()
	{
		vk::SurfaceCapabilitiesKHR surfaceCapabilities =
		    physicalDevice.getSurfaceCapabilitiesKHR(*surface);

		std::vector<vk::SurfaceFormatKHR> availableFormats =
		    physicalDevice.getSurfaceFormatsKHR(*surface);

		std::vector<vk::PresentModeKHR> availablePresentModes =
		    physicalDevice.getSurfacePresentModesKHR(*surface);

		swapChainExtent        = chooseSwapExtent(surfaceCapabilities);
		swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);
		uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

		vk::SwapchainCreateInfoKHR swapChainCreateInfo{
		    .surface          = *surface,
		    .minImageCount    = minImageCount,
		    .imageFormat      = swapChainSurfaceFormat.format,
		    .imageColorSpace  = swapChainSurfaceFormat.colorSpace,
		    .imageExtent      = swapChainExtent,
		    .imageArrayLayers = 1,
		    .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
		    .imageSharingMode = vk::SharingMode::eExclusive,
		    .preTransform     = surfaceCapabilities.currentTransform,
		    .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		    .presentMode      = chooseSwapPresentMode(availablePresentModes),
		    .clipped          = true};

		swapChain       = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
		swapChainImages = swapChain.getImages();
	};

	void cleanupSwapChain()
	{
		swapChainImageViews.clear();
		swapChain = nullptr;
	}

	void createLogicalDevice()
	{
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
		    physicalDevice.getQueueFamilyProperties();

		uint32_t queueIndex = ~0;

		for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
		{
			if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
			    physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
			{
				// found a queue family that supports both graphics and present
				queueIndex = qfpIndex;
				break;
			}
		}

		if (queueIndex == ~0)
		{
			throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
		}

		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
		                   vk::PhysicalDeviceVulkan13Features,
		                   vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
		    featureChain = {
		        {.features = {.samplerAnisotropy = true}},        // vk::PhysicalDeviceFeatures2
		        {.shaderDrawParameters = true},                   // Enable shader draw parameters from Vulkan 1.1
		        {.synchronization2 = true,
		         .dynamicRendering = true},           // Enable dynamic rendering from Vulkan 1.3
		        {.extendedDynamicState = true}        // Enable extended dynamic state from the extension
		    };

		float                     queuePriority = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
		    .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
		vk::DeviceCreateInfo deviceCreateInfo{.pNext                = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
		                                      .queueCreateInfoCount = 1,
		                                      .pQueueCreateInfos    = &deviceQueueCreateInfo,
		                                      .enabledExtensionCount =
		                                          static_cast<uint32_t>(requiredDeviceExtension.size()),
		                                      .ppEnabledExtensionNames = requiredDeviceExtension.data()};

		device             = vk::raii::Device(physicalDevice, deviceCreateInfo);
		graphicsQueueIndex = queueIndex;
		graphicsQueue      = vk::raii::Queue(device, queueIndex, 0);
	}

	void createSurface()
	{
		VkSurfaceKHR _surface{};

		if (!SDL_Vulkan_CreateSurface(window.getNativeWindow(), *instance, nullptr, &_surface))
		{
			throw std::runtime_error(SDL_GetError());
		}
		surface = vk::raii::SurfaceKHR(instance, _surface);
	}

	vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
	                               vk::FormatFeatureFlags features)
	{
		for (const auto format : candidates)
		{
			vk::FormatProperties props = physicalDevice.getFormatProperties(format);

			if (((tiling == vk::ImageTiling::eLinear) &&
			     ((props.linearTilingFeatures & features) == features)) ||
			    ((tiling == vk::ImageTiling::eOptimal) &&
			     ((props.optimalTilingFeatures & features) == features)))
			{
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	vk::Format findDepthFormat()
	{
		return findSupportedFormat(
		    {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
		    vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	}

	void createGraphicsPipeline()
	{
		vk::raii::ShaderModule            shaderModule = createShaderModule(readFile("/Users/caleb/repos/anjean/build/shaders/vulkan/shader.spv"));
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
		    .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
		    .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		auto                                   bindingDescription    = Vertex::getBindingDescription();
		auto                                   attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
		    .vertexBindingDescriptionCount   = 1,
		    .pVertexBindingDescriptions      = &bindingDescription,
		    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
		    .pVertexAttributeDescriptions    = attributeDescriptions.data()};

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology =
		                                                           vk::PrimitiveTopology::eTriangleList};
		vk::Viewport                             viewport{0.0f,
		                                                  0.0f,
		                                                  static_cast<float>(swapChainExtent.width),
		                                                  static_cast<float>(swapChainExtent.height),
		                                                  0.0f,
		                                                  1.0f};
		vk::Rect2D                               scissor{vk::Offset2D{0, 0}, swapChainExtent};

		std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
		                                               vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount =
		                                                    static_cast<uint32_t>(dynamicStates.size()),
		                                                .pDynamicStates = dynamicStates.data()};

		vk::PipelineViewportStateCreateInfo      viewportState{.viewportCount = 1, .scissorCount = 1};
		vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable        = vk::False,
		                                                    .rasterizerDiscardEnable = vk::False,
		                                                    .polygonMode             = vk::PolygonMode::eFill,
		                                                    .cullMode                = vk::CullModeFlagBits::eNone,
		                                                    .frontFace               = vk::FrontFace::eCounterClockwise,
		                                                    .depthBiasEnable         = vk::False,
		                                                    .lineWidth               = 1.0f};
		vk::PipelineMultisampleStateCreateInfo   multisampling{
		    .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		    .blendEnable         = vk::True,
		    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
		    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
		    .colorBlendOp        = vk::BlendOp::eAdd,
		    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
		    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
		    .alphaBlendOp        = vk::BlendOp::eAdd,
		    .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		                           vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
		vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable   = vk::False,
		                                                    .logicOp         = vk::LogicOp::eCopy,
		                                                    .attachmentCount = 1,
		                                                    .pAttachments    = &colorBlendAttachment};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
		    .setLayoutCount = 1, .pSetLayouts = &*descriptorSetLayout, .pushConstantRangeCount = 0};

		pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		vk::PipelineDepthStencilStateCreateInfo depthStencil{.depthTestEnable       = vk::True,
		                                                     .depthWriteEnable      = vk::True,
		                                                     .depthCompareOp        = vk::CompareOp::eLess,
		                                                     .depthBoundsTestEnable = vk::False,
		                                                     .stencilTestEnable     = vk::False};

		vk::Format depthFormat = findDepthFormat();

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo>
		    pipelineCreateInfoChain = {{
		                                   .stageCount          = 2,
		                                   .pStages             = shaderStages,
		                                   .pVertexInputState   = &vertexInputInfo,
		                                   .pInputAssemblyState = &inputAssembly,
		                                   .pViewportState      = &viewportState,
		                                   .pRasterizationState = &rasterizer,
		                                   .pMultisampleState   = &multisampling,
		                                   .pColorBlendState    = &colorBlending,
		                                   .pDynamicState       = &dynamicState,
		                                   .layout              = pipelineLayout,
		                                   .renderPass          = nullptr,
		                                   .pDepthStencilState  = &depthStencil,

		                               },
		                               {.colorAttachmentCount    = 1,
		                                .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
		                                .depthAttachmentFormat   = depthFormat}};

		graphicsPipeline = vk::raii::Pipeline(
		    device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}

	void createCommandPool()
	{
		vk::CommandPoolCreateInfo poolInfo{.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		                                   .queueFamilyIndex = graphicsQueueIndex};
		commandPool = vk::raii::CommandPool(device, poolInfo);
	}

	void createCommandBuffers()
	{
		vk::CommandBufferAllocateInfo allocInfo{.commandPool        = commandPool,
		                                        .level              = vk::CommandBufferLevel::ePrimary,
		                                        .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

		commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
	}

	void transition_image_layout(vk::Image image, vk::ImageLayout old_layout,
	                             vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask,
	                             vk::AccessFlags2        dst_access_mask,
	                             vk::PipelineStageFlags2 src_stage_mask,
	                             vk::PipelineStageFlags2 dst_stage_mask,
	                             vk::ImageAspectFlags    image_aspect_flags)
	{
		vk::ImageMemoryBarrier2 barrier         = {.srcStageMask        = src_stage_mask,
		                                           .srcAccessMask       = src_access_mask,
		                                           .dstStageMask        = dst_stage_mask,
		                                           .dstAccessMask       = dst_access_mask,
		                                           .oldLayout           = old_layout,
		                                           .newLayout           = new_layout,
		                                           .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		                                           .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		                                           .image               = image,
		                                           .subresourceRange    = {.aspectMask     = image_aspect_flags,
		                                                                   .baseMipLevel   = 0,
		                                                                   .levelCount     = 1,
		                                                                   .baseArrayLayer = 0,
		                                                                   .layerCount     = 1}};
		vk::DependencyInfo      dependency_info = {
		    .dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};
		commandBuffers[frameIndex].pipelineBarrier2(dependency_info);
	}

	void recordCommandBuffer(uint32_t imageIndex)
	{
		commandBuffers[frameIndex].begin({});

		// Transition the image layout for rendering
		transition_image_layout(
		    swapChainImages[imageIndex], vk::ImageLayout::eUndefined,
		    vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits2::eColorAttachmentWrite,
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);

		transition_image_layout(*depthImage, vk::ImageLayout::eUndefined,
		                        vk::ImageLayout::eDepthAttachmentOptimal,
		                        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		                        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		                        vk::PipelineStageFlagBits2::eEarlyFragmentTests |
		                            vk::PipelineStageFlagBits2::eLateFragmentTests,
		                        vk::PipelineStageFlagBits2::eEarlyFragmentTests |
		                            vk::PipelineStageFlagBits2::eLateFragmentTests,
		                        vk::ImageAspectFlagBits::eDepth);

		// Set up the color attachment
		vk::ClearValue              clearDepth     = vk::ClearDepthStencilValue(1.0f, 0);
		vk::RenderingAttachmentInfo attachmentInfo = {.imageView = swapChainImageViews[imageIndex],
		                                              .imageLayout =
		                                                  vk::ImageLayout::eColorAttachmentOptimal,
		                                              .loadOp     = vk::AttachmentLoadOp::eClear,
		                                              .storeOp    = vk::AttachmentStoreOp::eStore,
		                                              .clearValue = clearColor};

		vk::RenderingAttachmentInfo depthAttachmentInfo = {.imageView = depthImageView,
		                                                   .imageLayout =
		                                                       vk::ImageLayout::eDepthAttachmentOptimal,
		                                                   .loadOp     = vk::AttachmentLoadOp::eClear,
		                                                   .storeOp    = vk::AttachmentStoreOp::eDontCare,
		                                                   .clearValue = clearDepth};

		// Set up the rendering info
		vk::RenderingInfo renderingInfo = {.renderArea           = {.offset = {0, 0}, .extent = swapChainExtent},
		                                   .layerCount           = 1,
		                                   .colorAttachmentCount = 1,
		                                   .pColorAttachments    = &attachmentInfo,
		                                   .pDepthAttachment     = &depthAttachmentInfo};

		// Begin rendering
		commandBuffers[frameIndex].beginRendering(renderingInfo);

		// Rendering commands will go here
		commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
		commandBuffers[frameIndex].setViewport(
		    0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width),
		                    static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
		commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));

		for (uint32_t objectIndex = 0; objectIndex < renderObjects.size(); ++objectIndex)
		{
			const auto &renderObject = renderObjects[objectIndex];
			auto        meshIt       = gpuMeshes.find(renderObject.meshId);
			if (meshIt == gpuMeshes.end())
			{
				continue;
			}

			const GpuMesh &mesh = meshIt->second;

			commandBuffers[frameIndex].bindVertexBuffers(
			    0,
			    *mesh.vertexBuffer,
			    {0});

			commandBuffers[frameIndex].bindDescriptorSets(
			    vk::PipelineBindPoint::eGraphics,
			    *pipelineLayout,
			    0,
			    *renderObject.descriptorSets[frameIndex],
			    nullptr);

			if (mesh.indexCount > 0)
			{
				commandBuffers[frameIndex].bindIndexBuffer(*mesh.indexBuffer, 0, vk::IndexType::eUint32);
				commandBuffers[frameIndex].drawIndexed(mesh.indexCount, 1, 0, 0, objectIndex);
			}
			else
			{
				commandBuffers[frameIndex].draw(mesh.vertexCount, 1, 0, objectIndex);
			}
		}

		// End rendering
		commandBuffers[frameIndex].endRendering();

		// Transition the image layout for presentation
		transition_image_layout(
		    swapChainImages[imageIndex], vk::ImageLayout::eColorAttachmentOptimal,
		    vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits2::eColorAttachmentWrite, {},
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe,
		    vk::ImageAspectFlagBits::eColor);

		commandBuffers[frameIndex].end();
	}

	void createSyncObjects()
	{
		assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() &&
		       inFlightFences.empty());

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
			inFlightFences.emplace_back(device,
			                            vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
		}
	}

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
	{
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) &&
			    (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
	    createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		vk::BufferCreateInfo bufferInfo{
		    .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
		vk::raii::Buffer       buffer          = vk::raii::Buffer(device, bufferInfo);
		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{.allocationSize = memRequirements.size,
		                                 .memoryTypeIndex =
		                                     findMemoryType(memRequirements.memoryTypeBits, properties)};
		vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
		buffer.bindMemory(*bufferMemory, 0);
		return {std::move(buffer), std::move(bufferMemory)};
	}

	vk::raii::CommandBuffer beginSingleTimeCommands()
	{
		vk::CommandBufferAllocateInfo allocInfo{.commandPool        = commandPool,
		                                        .level              = vk::CommandBufferLevel::ePrimary,
		                                        .commandBufferCount = 1};
		vk::raii::CommandBuffer       commandBuffer =
		    std::move(vk::raii::CommandBuffers(device, allocInfo).front());

		vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
		commandBuffer.begin(beginInfo);

		return std::move(commandBuffer);
	}

	void endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer)
	{
		commandBuffer.end();

		vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
		graphicsQueue.submit(submitInfo, nullptr);
		graphicsQueue.waitIdle();
	}

	void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size)
	{
		vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
		commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{.size = size});
		endSingleTimeCommands(std::move(commandCopyBuffer));
	}

	void createDescriptorSetLayout()
	{
		std::array<vk::DescriptorSetLayoutBinding, 3> bindings{
		    {{.binding         = 0,
		      .descriptorType  = vk::DescriptorType::eUniformBuffer,
		      .descriptorCount = 1,
		      .stageFlags      = vk::ShaderStageFlagBits::eVertex},
		     {.binding         = 1,
		      .descriptorType  = vk::DescriptorType::eStorageBuffer,
		      .descriptorCount = 1,
		      .stageFlags      = vk::ShaderStageFlagBits::eVertex},
		     {.binding         = 2,
		      .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
		      .descriptorCount = 1,
		      .stageFlags      = vk::ShaderStageFlagBits::eFragment}}};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{
		    .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()};

		descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
	}

	void createFrameDataUBOs()
	{
		vk::DeviceSize bufferSize = sizeof(FrameDataUBO);

		frameDataUBOs.clear();
		frameDataUBOMemories.clear();
		frameDataUBOMapped.clear();

		frameDataUBOs.reserve(MAX_FRAMES_IN_FLIGHT);
		frameDataUBOMemories.reserve(MAX_FRAMES_IN_FLIGHT);
		frameDataUBOMapped.reserve(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto [buffer, bufferMem] = createBuffer(
			    bufferSize,
			    vk::BufferUsageFlagBits::eUniformBuffer,
			    vk::MemoryPropertyFlagBits::eHostVisible |
			        vk::MemoryPropertyFlagBits::eHostCoherent);

			frameDataUBOs.push_back(std::move(buffer));
			frameDataUBOMemories.push_back(std::move(bufferMem));

			frameDataUBOMapped.push_back(
			    frameDataUBOMemories.back().mapMemory(0, bufferSize));
		}
	}

	void updateFrameDataUBOs(uint32_t currentFrame)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto  currentTime = std::chrono::high_resolution_clock::now();
		float time        = std::chrono::duration<float>(currentTime - startTime).count();

		FrameDataUBO frameDataUBO{};

		frameDataUBO.frameIndex              = frameIndex;
		frameDataUBO.camera.projectionMatrix = activeCamera.projectionMatrix;
		frameDataUBO.camera.viewMatrix       = activeCamera.viewMatrix;
		frameDataUBO.time                    = time;
		frameDataUBO.renderSize              = glm::vec2(
		    swapChainExtent.width,
		    swapChainExtent.height);

		std::memcpy(
		    frameDataUBOMapped[currentFrame],
		    &frameDataUBO,
		    sizeof(FrameDataUBO));
	}

	void createObjectDataBuffers()
	{
		objectDataSSBO.clear();
		objectDataSSBOMemories.clear();
		objectDataSSBOMapped.clear();

		if (renderObjects.empty())
			return;

		vk::DeviceSize bufferSize =
		    sizeof(ObjectData) * renderObjects.size();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto [buffer, bufferMem] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eStorageBuffer,
			                                        vk::MemoryPropertyFlagBits::eHostVisible |
			                                            vk::MemoryPropertyFlagBits::eHostCoherent);

			objectDataSSBO.emplace_back(std::move(buffer));
			objectDataSSBOMemories.emplace_back(std::move(bufferMem));
			objectDataSSBOMapped.emplace_back(
			    objectDataSSBOMemories[i].mapMemory(0, bufferSize));
		}
	}

	void createDescriptorPool()
	{
		uint32_t descriptorCount =
		    static_cast<uint32_t>(renderObjects.size()) * MAX_FRAMES_IN_FLIGHT;

		if (descriptorCount == 0)
		{
			descriptorPool = nullptr;
			return;
		}

		std::array<vk::DescriptorPoolSize, 3> poolSize{
		    {{.type            = vk::DescriptorType::eUniformBuffer,
		      .descriptorCount = descriptorCount},
		     {.type            = vk::DescriptorType::eStorageBuffer,
		      .descriptorCount = descriptorCount},
		     {.type            = vk::DescriptorType::eCombinedImageSampler,
		      .descriptorCount = descriptorCount}}};
		vk::DescriptorPoolCreateInfo poolInfo{.flags =
		                                          vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		                                      .maxSets       = descriptorCount,
		                                      .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
		                                      .pPoolSizes    = poolSize.data()};

		descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
	}

	void createDescriptorSets()
	{
		for (auto &renderObject : renderObjects)
		{
			std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
			vk::DescriptorSetAllocateInfo        allocInfo{.descriptorPool = descriptorPool,
			                                               .descriptorSetCount =
			                                                   static_cast<uint32_t>(layouts.size()),
			                                               .pSetLayouts = layouts.data()};

			renderObject.descriptorSets.clear();
			renderObject.descriptorSets = device.allocateDescriptorSets(allocInfo);

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vk::DescriptorBufferInfo frameDataBufferInfo{.buffer = *frameDataUBOs[i],
				                                             .offset = 0,
				                                             .range  = sizeof(FrameDataUBO)};
				vk::DescriptorBufferInfo bufferInfo{.buffer = *objectDataSSBO[i],
				                                    .offset = 0,
				                                    .range  = sizeof(ObjectData) * renderObjects.size()};
				vk::DescriptorImageInfo  imageInfo{.sampler     = textureSampler,
				                                   .imageView   = textureImageView,
				                                   .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

				std::array<vk::WriteDescriptorSet, 3> descriptorWrites{
				    {{.dstSet          = *renderObject.descriptorSets[i],
				      .dstBinding      = 0,
				      .dstArrayElement = 0,
				      .descriptorCount = 1,
				      .descriptorType  = vk::DescriptorType::eUniformBuffer,
				      .pBufferInfo     = &frameDataBufferInfo},
				     {.dstSet          = *renderObject.descriptorSets[i],
				      .dstBinding      = 1,
				      .dstArrayElement = 0,
				      .descriptorCount = 1,
				      .descriptorType  = vk::DescriptorType::eStorageBuffer,
				      .pBufferInfo     = &bufferInfo},
				     {.dstSet          = *renderObject.descriptorSets[i],
				      .dstBinding      = 2,
				      .dstArrayElement = 0,
				      .descriptorCount = 1,
				      .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
				      .pImageInfo      = &imageInfo}}};
				device.updateDescriptorSets(descriptorWrites, {});
			}
		}
	}

	std::pair<vk::raii::Image, vk::raii::DeviceMemory>
	    createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
	                vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		vk::ImageCreateInfo imageInfo{.imageType   = vk::ImageType::e2D,
		                              .format      = format,
		                              .extent      = {width, height, 1},
		                              .mipLevels   = 1,
		                              .arrayLayers = 1,
		                              .samples     = vk::SampleCountFlagBits::e1,
		                              .tiling      = tiling,
		                              .usage       = usage,
		                              .sharingMode = vk::SharingMode::eExclusive};

		vk::raii::Image image = vk::raii::Image(device, imageInfo);

		vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{.allocationSize = memRequirements.size,
		                                 .memoryTypeIndex =
		                                     findMemoryType(memRequirements.memoryTypeBits, properties)};
		vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(device, allocInfo);
		image.bindMemory(imageMemory, 0);

		return {std::move(image), std::move(imageMemory)};
	}

	void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image,
	                           vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
	{
		vk::ImageMemoryBarrier barrier{.oldLayout           = oldLayout,
		                               .newLayout           = newLayout,
		                               .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
		                               .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
		                               .image               = image,
		                               .subresourceRange    = {.aspectMask = vk::ImageAspectFlagBits::eColor,
		                                                       .levelCount = 1,
		                                                       .layerCount = 1}};

		if (oldLayout == vk::ImageLayout::eUndefined &&
		    newLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
		         newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage      = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}
		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
	}

	void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer,
	                       vk::raii::Image &image, uint32_t width, uint32_t height)
	{
		vk::BufferImageCopy region{.bufferOffset      = 0,
		                           .bufferRowLength   = 0,
		                           .bufferImageHeight = 0,
		                           .imageSubresource  = {.aspectMask     = vk::ImageAspectFlagBits::eColor,
		                                                 .mipLevel       = 0,
		                                                 .baseArrayLayer = 0,
		                                                 .layerCount     = 1},
		                           .imageOffset       = {0, 0, 0},
		                           .imageExtent       = {width, height, 1}};

		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
	}

	void createTextureImage()
	{
		ktxTexture    *kTexture;
		KTX_error_code result = ktxTexture_CreateFromNamedFile(
		    TEXTURE_PATH.c_str(),
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

		auto [stagingBuffer, stagingBufferMemory] = createBuffer(
		    imageSize, vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void *data = stagingBufferMemory.mapMemory(0, imageSize);
		memcpy(data, ktxTextureData, imageSize);
		stagingBufferMemory.unmapMemory();

		vk::Format textureFormat = vk::Format::eR8G8B8A8Srgb;        // Default format, should be determined from KTX metadata

		std::tie(textureImage, textureImageMemory) =
		    createImage(texWidth, texHeight, textureFormat, vk::ImageTiling::eOptimal,
		                vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		                vk::MemoryPropertyFlagBits::eDeviceLocal);

		vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();
		transitionImageLayout(commandBuffer, textureImage, vk::ImageLayout::eUndefined,
		                      vk::ImageLayout::eTransferDstOptimal);
		copyBufferToImage(commandBuffer, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
		                  static_cast<uint32_t>(texHeight));

		transitionImageLayout(commandBuffer, textureImage, vk::ImageLayout::eTransferDstOptimal,
		                      vk::ImageLayout::eShaderReadOnlyOptimal);
		ktxTexture_Destroy(kTexture);
		endSingleTimeCommands(std::move(commandBuffer));
	}

	vk::raii::ImageView createImageView(const vk::Image &image, vk::Format format,
	                                    vk::ImageAspectFlags aspectFlags)
	{
		vk::ImageViewCreateInfo viewInfo{.image            = image,
		                                 .viewType         = vk::ImageViewType::e2D,
		                                 .format           = format,
		                                 .subresourceRange = {.aspectMask     = aspectFlags,
		                                                      .baseMipLevel   = 0,
		                                                      .levelCount     = 1,
		                                                      .baseArrayLayer = 0,
		                                                      .layerCount     = 1}};
		return vk::raii::ImageView(device, viewInfo);
	}

	void createImageViews()
	{
		assert(swapChainImageViews.empty());

		swapChainImageViews.reserve(swapChainImages.size());
		for (auto &image : swapChainImages)
		{
			swapChainImageViews.emplace_back(
			    createImageView(image, swapChainSurfaceFormat.format, vk::ImageAspectFlagBits::eColor));
		}
	}

	void createTextureImageView()
	{
		textureImageView =
		    createImageView(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	}

	void createDepthResources()
	{
		vk::Format depthFormat = findDepthFormat();

		std::tie(depthImage, depthImageMemory) = createImage(
		    swapChainExtent.width, swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal,
		    vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
		depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
	}

	void recreateSwapChain()
	{
		int width  = 0;
		int height = 0;

		SDL_GetWindowSizeInPixels(window.getNativeWindow(), &width, &height);

		while (width == 0 || height == 0)
		{
			SDL_WaitEvent(nullptr);
			SDL_GetWindowSizeInPixels(window.getNativeWindow(), &width, &height);
		}

		device.waitIdle();

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createDepthResources();
	}

	void createTextureSampler()
	{
		vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
		vk::SamplerCreateInfo        samplerInfo{.magFilter        = vk::Filter::eLinear,
		                                         .minFilter        = vk::Filter::eLinear,
		                                         .mipmapMode       = vk::SamplerMipmapMode::eLinear,
		                                         .addressModeU     = vk::SamplerAddressMode::eRepeat,
		                                         .addressModeV     = vk::SamplerAddressMode::eRepeat,
		                                         .addressModeW     = vk::SamplerAddressMode::eRepeat,
		                                         .anisotropyEnable = vk::True,
		                                         .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
		                                         .compareEnable    = vk::False,
		                                         .compareOp        = vk::CompareOp::eAlways};
		samplerInfo.borderColor             = vk::BorderColor::eIntOpaqueBlack;
		samplerInfo.unnormalizedCoordinates = vk::False;
		samplerInfo.mipLodBias              = 0.0f;
		samplerInfo.minLod                  = 0.0f;
		samplerInfo.maxLod                  = 0.0f;

		textureSampler = vk::raii::Sampler(device, samplerInfo);
	}

	bool needsRenderObjectRebuild(std::span<const Anjean::Core::RenderPacket> packets)
	{
		if (renderObjects.size() != packets.size())
			return true;

		for (size_t i = 0; i < packets.size(); ++i)
		{
			if (renderObjects[i].objectId != packets[i].objectId)
				return true;

			if (renderObjects[i].meshId != packets[i].meshId)
				return true;
		}

		return false;
	}

	void rebuildRenderObjects(std::span<const Anjean::Core::RenderPacket> packets)
	{
		device.waitIdle();

		cleanupObjectUniformsAndDescriptors();
		renderObjects.clear();

		renderObjects.reserve(packets.size());

		for (const auto &packet : packets)
		{
			if (!gpuMeshes.contains(packet.meshId))
			{
				continue;
			}

			RenderObject object{};
			object.objectId = packet.objectId;
			object.meshId   = packet.meshId;
			object.position = packet.position;
			object.rotation = packet.rotation;
			object.scale    = packet.scale;

			renderObjects.push_back(std::move(object));
		}

		if (renderObjects.empty())
		{
			return;
		}

		createObjectDataBuffers();
		createDescriptorPool();
		createDescriptorSets();
	}

	// Initialize the game objects with different positions, rotations, and scales
	void setRenderPackets(std::span<const Anjean::Core::RenderPacket> packets)
	{
		if (needsRenderObjectRebuild(packets))
		{
			rebuildRenderObjects(packets);
			return;
		}

		for (size_t i = 0; i < packets.size(); ++i)
		{
			renderObjects[i].position = packets[i].position;
			renderObjects[i].rotation = packets[i].rotation;
			renderObjects[i].scale    = packets[i].scale;
		}
	}

	void cleanupObjectUniformsAndDescriptors()
	{
		for (auto &renderObject : renderObjects)
		{
			renderObject.descriptorSets.clear();
		}

		descriptorPool = nullptr;
	}

	void updateObjectDataSSBO(uint32_t currentFrame)
	{
		if (renderObjects.empty())
		{
			return;
		}

		if (objectDataSSBOMapped.size() <= currentFrame || objectDataSSBOMapped[currentFrame] == nullptr)
		{
			throw std::runtime_error("ObjectData SSBO is not initialized for current frame.");
		}

		std::vector<ObjectData> objectData(renderObjects.size());

		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			objectData[i].modelMatrix =
			    renderObjects[i].getModelMatrix();
		}

		std::memcpy(
		    objectDataSSBOMapped[currentFrame],
		    objectData.data(),
		    sizeof(ObjectData) * objectData.size());
	}

	void initVulkan()
	{
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();

		createDescriptorSetLayout();
		createGraphicsPipeline();

		createCommandPool();
		createDepthResources();

		createTextureImage();
		createTextureImageView();
		createTextureSampler();

		createFrameDataUBOs();

		createCommandBuffers();
		createSyncObjects();
	}

	void drawFrame()
	{
		auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);

		if (fenceResult != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to wait for fence!");
		}

		auto [result, imageIndex] =
		    swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapChain();
			return;
		}
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		// Only reset the fence if we are submitting work
		device.resetFences(*inFlightFences[frameIndex]);

		updateFrameDataUBOs(frameIndex);
		updateObjectDataSSBO(frameIndex);

		commandBuffers[frameIndex].reset();
		recordCommandBuffer(imageIndex);

		vk::PipelineStageFlags waitDestinationStageMask(
		    vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitInfo{.waitSemaphoreCount   = 1,
		                                .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
		                                .pWaitDstStageMask    = &waitDestinationStageMask,
		                                .commandBufferCount   = 1,
		                                .pCommandBuffers      = &*commandBuffers[frameIndex],
		                                .signalSemaphoreCount = 1,
		                                .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]};
		graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

		const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
		                                        .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
		                                        .swapchainCount     = 1,
		                                        .pSwapchains        = &*swapChain,
		                                        .pImageIndices      = &imageIndex};

		// presentInfoKHR.pResults = nullptr;        // Optional
		result = graphicsQueue.presentKHR(presentInfoKHR);

		if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) ||
		    framebufferResized)
		{
			recreateSwapChain();
		}
		else
		{
			// There are no other success codes than eSuccess; on any error code, presentKHR already threw
			// an exception.
			assert(result == vk::Result::eSuccess);
		}

		frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void cleanup()
	{
		cleanupSwapChain();

		// SDL_DestroyWindow(window);
		// SDL_Quit();
	}

	void uploadMeshToGPU(const Anjean::Core::MeshData &meshData)
	{
		std::vector<Vertex> vertices;
		vertices.reserve(meshData.vertices.size());

		for (const auto &v : meshData.vertices)
		{
			Vertex vertex{};

			vertex.pos = {
			    v.position.x,
			    v.position.y,
			    v.position.z};

			vertex.texCoord = {
			    v.uv.x,
			    v.uv.y};

			vertex.normal = {0.0f, 0.0f, 1.0f};        // temporary until Core::MeshVertex has normals
			vertex.color  = {1.0f, 1.0f, 1.0f};

			vertices.push_back(vertex);
		}

		const auto &indices = meshData.indices;

		GpuMesh gpuMesh{};
		gpuMesh.id          = meshData.id;
		gpuMesh.indexCount  = static_cast<uint32_t>(indices.size());
		gpuMesh.vertexCount = static_cast<uint32_t>(vertices.size());

		createVertexBufferForMesh(vertices, gpuMesh);
		createIndexBufferForMesh(indices, gpuMesh);

		gpuMeshes[meshData.id] = std::move(gpuMesh);
	}

	void createVertexBufferForMesh(const std::vector<Vertex> &vertices, GpuMesh &mesh)
	{
		vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		auto [stagingBuffer, stagingBufferMemory] = createBuffer(
		    bufferSize,
		    vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible |
		        vk::MemoryPropertyFlagBits::eHostCoherent);

		void *data = stagingBufferMemory.mapMemory(0, bufferSize);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		stagingBufferMemory.unmapMemory();

		std::tie(mesh.vertexBuffer, mesh.vertexBufferMemory) = createBuffer(
		    bufferSize,
		    vk::BufferUsageFlagBits::eVertexBuffer |
		        vk::BufferUsageFlagBits::eTransferDst,
		    vk::MemoryPropertyFlagBits::eDeviceLocal);

		copyBuffer(stagingBuffer, mesh.vertexBuffer, bufferSize);
	}

	void createIndexBufferForMesh(const std::vector<uint32_t> &indices, GpuMesh &mesh)
	{
		if (indices.empty())
		{
			mesh.indexCount = 0;
			return;
		}

		vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		auto [stagingBuffer, stagingBufferMemory] = createBuffer(
		    bufferSize,
		    vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible |
		        vk::MemoryPropertyFlagBits::eHostCoherent);

		void *data = stagingBufferMemory.mapMemory(0, bufferSize);
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		stagingBufferMemory.unmapMemory();

		std::tie(mesh.indexBuffer, mesh.indexBufferMemory) = createBuffer(
		    bufferSize,
		    vk::BufferUsageFlagBits::eIndexBuffer |
		        vk::BufferUsageFlagBits::eTransferDst,
		    vk::MemoryPropertyFlagBits::eDeviceLocal);

		copyBuffer(stagingBuffer, mesh.indexBuffer, bufferSize);
	}
};

VulkanRendererBackend::VulkanRendererBackend(Anjean::Window &window) : m_impl(std::make_unique<Impl>(window))
{
	m_impl->initVulkan();
}

VulkanRendererBackend::~VulkanRendererBackend() = default;

void VulkanRendererBackend::beginFrame(const Color &clearColor)
{}

TextureHandle VulkanRendererBackend::createTexture(TextureDesc &desc)
{
	return {0};
}

PipelineHandle VulkanRendererBackend::createPipeline(const PipelineDesc &desc)
{
	return {0};
}

BufferHandle VulkanRendererBackend::createBuffer(const BufferDesc &desc)
{
	// TODO: real Vulkan buffer creation
	return {};
}

void VulkanRendererBackend::drawSprite(const PipelineHandle &pPipeline, const Core::MeshData &pMesh,
                                       const std::optional<TextureHandle> &pTexture,
                                       const ObjectUniform                &pObjectUniform)
{}

std::pair<decltype(BufferHandle::id), std::optional<decltype(TextureHandle::id)>>
    VulkanRendererBackend::loadMeshToGPU(Anjean::Core::MeshData pMesh)
{
	m_impl->uploadMeshToGPU(pMesh);

	return {
	    static_cast<decltype(BufferHandle::id)>(pMesh.id),
	    std::nullopt};
}

void VulkanRendererBackend::endFrame()
{}

void VulkanRendererBackend::onResize(int width, int height)
{
	m_impl->framebufferResized = true;
}

void VulkanRendererBackend::renderFrame(const Anjean::Core::CameraPacket &cameraPacket, const Color &clearColor, std::span<const Anjean::Core::RenderPacket> packets)
{
	CameraObject cameraObject{};

	if (cameraPacket.objectId)
	{
		const glm::vec3 position = cameraPacket.position;
		const glm::vec3 euler    = cameraPacket.rotation;

		const glm::quat qx = glm::angleAxis(euler.x, glm::vec3(1.0f, 0.0f, 0.0f));
		const glm::quat qy = glm::angleAxis(euler.y, glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::quat qz = glm::angleAxis(euler.z, glm::vec3(0.0f, 0.0f, 1.0f));
		const glm::quat q  = qz * qy * qx;

		const glm::mat4 T            = glm::translate(glm::mat4(1.0f), position);
		const glm::mat4 R            = glm::mat4_cast(q);
		const glm::mat4 worldNoScale = T * R;

		cameraObject.viewMatrix = glm::inverse(worldNoScale);

		cameraObject.projectionMatrix = glm::perspective(glm::radians(cameraPacket.fieldOfView), static_cast<float>(m_impl->swapChainExtent.width) / static_cast<float>(m_impl->swapChainExtent.height), cameraPacket.nearPlane, cameraPacket.farPlane);
		cameraObject.projectionMatrix[1][1] *= -1;
	}
	m_impl->activeCamera = cameraObject;
	m_impl->clearColor   = vk::ClearColorValue(clearColor.r, clearColor.g, clearColor.b, clearColor.a);

	m_impl->setRenderPackets(packets);

	m_impl->drawFrame();
}
}        // namespace Anjean::Rendering