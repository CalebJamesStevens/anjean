#include "../../../runtime/RuntimeTypes.h"
#include "../../../window/Window.h"
#include "MetalRendererBackend.h"

#include <SDL3/SDL_metal.h>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <stb_image.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Anjean::Rendering
{
namespace
{
static NS::String *MakeNSString(const char *text)
{
	return NS::String::string(text ? text : "", NS::UTF8StringEncoding);
}

static NS::String *MakeNSString(const std::string &text)
{
	return MakeNSString(text.c_str());
}

static const char *ErrorMessage(NS::Error *error, const char *fallback)
{
	if (!error || !error->localizedDescription())
	{
		return fallback;
	}

	return error->localizedDescription()->utf8String();
}

// CPU-side layout for the shader baseline:
//
// vertex buffer(1): ObjectUniforms { float4x4 model; }
// vertex buffer(2): FrameUniforms  { float4x4 viewProjection; }
// fragment buffer(0): MaterialUniforms { float4 baseColorFactor; }
//
// Important: use float4 for CPU-authored material constants. The shader can
// convert to half4 internally for color math.
struct GpuFrameUniforms
{
	float viewProjection[16];
};

struct GpuMaterialUniforms
{
	float baseColorFactor[4];
};

static GpuFrameUniforms IdentityFrameUniforms()
{
	return GpuFrameUniforms{{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	                         0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
}

static GpuMaterialUniforms WhiteMaterialUniforms()
{
	return GpuMaterialUniforms{{1.0f, 1.0f, 1.0f, 1.0f}};
}

template <typename T>
static bool HasBaseColorTexture(const T &desc)
{
	if constexpr (requires { desc.hasBaseColorTexture; })
	{
		return desc.hasBaseColorTexture;
	}
	else
	{
		return false;
	}
}

template <typename T>
static bool IsDepthEnabled(const T &desc)
{
	if constexpr (requires { desc.depthEnabled; })
	{
		return desc.depthEnabled;
	}
	else
	{
		return true;
	}
}

template <typename T>
static bool IsBlendingEnabled(const T &desc)
{
	if constexpr (requires { desc.blendingEnabled; })
	{
		return desc.blendingEnabled;
	}
	else
	{
		return true;
	}
}
}        // namespace

struct MetalRendererBackend::Impl
{
	SDL_MetalView metalView = nullptr;

	CA::MetalLayer *layer  = nullptr;
	MTL::Device    *device = nullptr;

	MTL4::CommandQueue     *commandQueue     = nullptr;
	MTL4::CommandBuffer    *commandBuffer    = nullptr;
	MTL4::CommandAllocator *commandAllocator = nullptr;

	CA::MetalDrawable          *drawable      = nullptr;
	MTL4::RenderCommandEncoder *renderEncoder = nullptr;

	MTL::Library   *shaderLibrary = nullptr;
	MTL4::Compiler *compiler      = nullptr;

	std::vector<MTL::Buffer *>              buffers;
	std::vector<MTL::Texture *>             textures;
	std::vector<MTL::RenderPipelineState *> pipelines;
	std::vector<bool>                       pipelineHasBaseColorTexture;

	std::vector<MTL4::ArgumentTable *> vertexArgumentTables;
	std::vector<MTL4::ArgumentTable *> fragmentArgumentTables;
	std::uint32_t                      nextVertexArgumentTable   = 0;
	std::uint32_t                      nextFragmentArgumentTable = 0;

	MTL::ResidencySet *residencySet = nullptr;

	MTL::Texture           *depthTexture   = nullptr;
	MTL::DepthStencilState *depthState     = nullptr;
	MTL::SamplerState      *nearestSampler = nullptr;

	BufferHandle  frameUniformBuffer{};
	BufferHandle  defaultMaterialUniformBuffer{};
	std::uint32_t depthTextureWidth  = 0;
	std::uint32_t depthTextureHeight = 0;

	std::vector<MTL::Buffer *> frameUploadBuffers;

	using Clock                 = std::chrono::steady_clock;
	Clock::time_point startTime = Clock::now();

	MTL::Buffer *createFrameUploadBuffer(const void *data, std::size_t size)
	{
		if (!data || size == 0)
		{
			throw std::runtime_error("Cannot create frame upload buffer with empty data.");
		}

		MTL::Buffer *buffer = device->newBuffer(data, size, MTL::ResourceStorageModeShared);

		if (!buffer)
		{
			throw std::runtime_error("Failed to create Metal frame upload buffer.");
		}

		if (residencySet)
		{
			residencySet->addAllocation(buffer);
			residencySet->commit();
		}

		frameUploadBuffers.push_back(buffer);
		return buffer;
	}

	BufferHandle addBuffer(MTL::Buffer *buffer)
	{
		buffers.push_back(buffer);
		return BufferHandle{static_cast<decltype(BufferHandle::id)>(buffers.size())};
	}

	TextureHandle addTexture(MTL::Texture *texture)
	{
		textures.push_back(texture);
		return TextureHandle{static_cast<decltype(TextureHandle::id)>(textures.size())};
	}

	PipelineHandle addPipeline(MTL::RenderPipelineState *pipeline, bool usesBaseColorTexture)
	{
		pipelines.push_back(pipeline);
		pipelineHasBaseColorTexture.push_back(usesBaseColorTexture);
		return PipelineHandle{static_cast<decltype(PipelineHandle::id)>(pipelines.size())};
	}

	MTL::Buffer *getBuffer(BufferHandle handle)
	{
		if (handle.id == 0 || handle.id > buffers.size())
		{
			throw std::runtime_error("Invalid Metal buffer handle.");
		}

		MTL::Buffer *buffer = buffers.at(handle.id - 1);

		if (!buffer)
		{
			throw std::runtime_error("Metal buffer handle points to a released buffer.");
		}

		return buffer;
	}

	MTL::Texture *getTexture(TextureHandle handle)
	{
		if (handle.id == 0 || handle.id > textures.size())
		{
			throw std::runtime_error("Invalid Metal texture handle.");
		}

		MTL::Texture *texture = textures.at(handle.id - 1);

		if (!texture)
		{
			throw std::runtime_error("Metal texture handle points to a released texture.");
		}

		return texture;
	}

	MTL::RenderPipelineState *getPipeline(PipelineHandle handle)
	{
		if (handle.id == 0 || handle.id > pipelines.size())
		{
			throw std::runtime_error("Invalid Metal pipeline handle.");
		}

		MTL::RenderPipelineState *pipeline = pipelines.at(handle.id - 1);

		if (!pipeline)
		{
			throw std::runtime_error("Metal pipeline handle points to a released pipeline.");
		}

		return pipeline;
	}

	bool pipelineRequiresBaseColorTexture(PipelineHandle handle) const
	{
		if (handle.id == 0 || handle.id > pipelineHasBaseColorTexture.size())
		{
			throw std::runtime_error("Invalid Metal pipeline handle.");
		}

		return pipelineHasBaseColorTexture.at(handle.id - 1);
	}

	BufferHandle createResidentBuffer(const void *data, std::size_t size,
	                                  const char *failureMessage)
	{
		if (!data || size == 0)
		{
			throw std::runtime_error("Cannot create buffer with empty data.");
		}

		MTL::Buffer *buffer = device->newBuffer(data, size, MTL::ResourceStorageModeShared);

		if (!buffer)
		{
			throw std::runtime_error(failureMessage);
		}

		residencySet->addAllocation(buffer);
		residencySet->commit();

		return addBuffer(buffer);
	}

	void createDefaultBuffers()
	{
		GpuFrameUniforms frame = IdentityFrameUniforms();
		frameUniformBuffer =
		    createResidentBuffer(&frame, sizeof(frame), "Failed to create frame uniform buffer.");

		GpuMaterialUniforms material = WhiteMaterialUniforms();
		defaultMaterialUniformBuffer = createResidentBuffer(
		    &material, sizeof(material), "Failed to create default material uniform buffer.");
	}

	void createDepthState()
	{
		auto *depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
		depthDesc->setDepthWriteEnabled(true);

		depthState = device->newDepthStencilState(depthDesc);
		depthDesc->release();

		if (!depthState)
		{
			throw std::runtime_error("Failed to create Metal depth-stencil state.");
		}
	}

	void createDefaultSampler()
	{
		auto *samplerDesc = MTL::SamplerDescriptor::alloc()->init();

		samplerDesc->setMinFilter(MTL::SamplerMinMagFilterNearest);
		samplerDesc->setMagFilter(MTL::SamplerMinMagFilterNearest);
		samplerDesc->setMipFilter(MTL::SamplerMipFilterNotMipmapped);
		samplerDesc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		samplerDesc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);

		nearestSampler = device->newSamplerState(samplerDesc);
		samplerDesc->release();

		if (!nearestSampler)
		{
			throw std::runtime_error("Failed to create Metal sampler state.");
		}
	}

	void ensureDepthTexture(std::uint32_t width, std::uint32_t height)
	{
		width  = std::max<std::uint32_t>(1, width);
		height = std::max<std::uint32_t>(1, height);

		if (depthTexture && depthTextureWidth == width && depthTextureHeight == height)
		{
			return;
		}

		if (depthTexture)
		{
			depthTexture->release();
			depthTexture = nullptr;
		}

		auto *depthTexDesc = MTL::TextureDescriptor::alloc()->init();

		depthTexDesc->setTextureType(MTL::TextureType2D);
		depthTexDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
		depthTexDesc->setWidth(width);
		depthTexDesc->setHeight(height);
		depthTexDesc->setStorageMode(MTL::StorageModePrivate);
		depthTexDesc->setUsage(MTL::TextureUsageRenderTarget);

		depthTexture = device->newTexture(depthTexDesc);
		depthTexDesc->release();

		if (!depthTexture)
		{
			throw std::runtime_error("Failed to create Metal depth texture.");
		}

		residencySet->addAllocation(depthTexture);
		residencySet->commit();

		depthTextureWidth  = width;
		depthTextureHeight = height;
	}

	void resetFrameArgumentTables()
	{
		nextVertexArgumentTable   = 0;
		nextFragmentArgumentTable = 0;
	}

	MTL4::ArgumentTable *createVertexArgumentTable()
	{
		auto *tableDesc = MTL4::ArgumentTableDescriptor::alloc()->init();
		tableDesc->setMaxBufferBindCount(3);

		MTL4::ArgumentTable *table = device->newArgumentTable(tableDesc, nullptr);
		tableDesc->release();

		if (!table)
		{
			throw std::runtime_error("Failed to create vertex argument table.");
		}

		return table;
	}

	MTL4::ArgumentTable *createFragmentArgumentTable()
	{
		auto *tableDesc = MTL4::ArgumentTableDescriptor::alloc()->init();
		tableDesc->setMaxBufferBindCount(1);
		tableDesc->setMaxTextureBindCount(1);
		tableDesc->setMaxSamplerStateBindCount(1);

		MTL4::ArgumentTable *table = device->newArgumentTable(tableDesc, nullptr);
		tableDesc->release();

		if (!table)
		{
			throw std::runtime_error("Failed to create fragment argument table.");
		}

		return table;
	}

	MTL4::ArgumentTable *acquireVertexArgumentTable()
	{
		if (nextVertexArgumentTable == vertexArgumentTables.size())
		{
			vertexArgumentTables.push_back(createVertexArgumentTable());
		}

		return vertexArgumentTables.at(nextVertexArgumentTable++);
	}

	MTL4::ArgumentTable *acquireFragmentArgumentTable()
	{
		if (nextFragmentArgumentTable == fragmentArgumentTables.size())
		{
			fragmentArgumentTables.push_back(createFragmentArgumentTable());
		}

		return fragmentArgumentTables.at(nextFragmentArgumentTable++);
	}

	void releaseFrameObjects()
	{
		if (renderEncoder)
		{
			renderEncoder->release();
			renderEncoder = nullptr;
		}

		if (drawable)
		{
			drawable->release();
			drawable = nullptr;
		}

		for (auto *buffer : frameUploadBuffers)
		{
			if (buffer)
			{
				buffer->release();
			}
		}
		frameUploadBuffers.clear();
	}

	void destroy()
	{
		releaseFrameObjects();

		for (auto *table : vertexArgumentTables)
		{
			if (table)
			{
				table->release();
			}
		}
		vertexArgumentTables.clear();

		for (auto *table : fragmentArgumentTables)
		{
			if (table)
			{
				table->release();
			}
		}
		fragmentArgumentTables.clear();

		for (auto *pipeline : pipelines)
		{
			if (pipeline)
			{
				pipeline->release();
			}
		}
		pipelines.clear();
		pipelineHasBaseColorTexture.clear();

		for (auto *buffer : buffers)
		{
			if (buffer)
			{
				buffer->release();
			}
		}
		buffers.clear();

		for (auto *texture : textures)
		{
			if (texture)
			{
				texture->release();
			}
		}
		textures.clear();

		if (depthTexture)
		{
			depthTexture->release();
			depthTexture = nullptr;
		}

		if (depthState)
		{
			depthState->release();
			depthState = nullptr;
		}

		if (nearestSampler)
		{
			nearestSampler->release();
			nearestSampler = nullptr;
		}

		if (compiler)
		{
			compiler->release();
			compiler = nullptr;
		}

		if (residencySet)
		{
			residencySet->release();
			residencySet = nullptr;
		}

		if (shaderLibrary)
		{
			shaderLibrary->release();
			shaderLibrary = nullptr;
		}

		if (commandBuffer)
		{
			commandBuffer->release();
			commandBuffer = nullptr;
		}

		if (commandAllocator)
		{
			commandAllocator->release();
			commandAllocator = nullptr;
		}

		if (commandQueue)
		{
			commandQueue->release();
			commandQueue = nullptr;
		}

		if (metalView)
		{
			SDL_Metal_DestroyView(metalView);
			metalView = nullptr;
			layer     = nullptr;
		}

		if (device)
		{
			device->release();
			device = nullptr;
		}
	}

	void ensureShaderLibrary()
	{
		if (shaderLibrary)
		{
			return;
		}

		shaderLibrary = device->newLibrary(MakeNSString(ANJEAN_METAL_SHADER_LIBRARY), nullptr);

		if (!shaderLibrary)
		{
			throw std::runtime_error("Failed to load Metal shader library.");
		}
	}

	void ensureCompiler()
	{
		if (compiler)
		{
			return;
		}

		auto *compilerDesc = MTL4::CompilerDescriptor::alloc()->init();
		compiler           = device->newCompiler(compilerDesc, nullptr);
		compilerDesc->release();

		if (!compiler)
		{
			throw std::runtime_error("Failed to create Metal 4 compiler.");
		}
	}

	PipelineHandle createPipeline(const PipelineDesc &desc)
	{
		ensureShaderLibrary();
		ensureCompiler();

		const bool hasBaseColorTexture = HasBaseColorTexture(desc);
		const bool depthEnabled        = IsDepthEnabled(desc);
		const bool blendingEnabled     = IsBlendingEnabled(desc);

		auto *vertexFunction = MTL4::LibraryFunctionDescriptor::alloc()->init();
		vertexFunction->setLibrary(shaderLibrary);
		vertexFunction->setName(MakeNSString(desc.vertexFunction));

		auto *fragmentBaseFunction = MTL4::LibraryFunctionDescriptor::alloc()->init();
		fragmentBaseFunction->setLibrary(shaderLibrary);
		fragmentBaseFunction->setName(MakeNSString(desc.fragmentFunction));

		auto *constants     = MTL::FunctionConstantValues::alloc()->init();
		bool  constantValue = hasBaseColorTexture;
		constants->setConstantValue(&constantValue, MTL::DataTypeBool, static_cast<NS::UInteger>(0));

		auto *fragmentFunction = MTL4::SpecializedFunctionDescriptor::alloc()->init();
		fragmentFunction->setFunctionDescriptor(fragmentBaseFunction);
		fragmentFunction->setConstantValues(constants);

		std::string specializedName =
		    std::string(desc.fragmentFunction) + (hasBaseColorTexture ? "_textured" : "_solid");
		fragmentFunction->setSpecializedName(MakeNSString(specializedName));

		auto *pipelineDesc = MTL4::RenderPipelineDescriptor::alloc()->init();
		pipelineDesc->setLabel(
		    MakeNSString(desc.debugName ? desc.debugName : specializedName.c_str()));
		pipelineDesc->setVertexFunctionDescriptor(vertexFunction);
		pipelineDesc->setFragmentFunctionDescriptor(fragmentFunction);

		auto *colorAttachment = pipelineDesc->colorAttachments()->object(0);
		colorAttachment->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

		if (blendingEnabled)
		{
			colorAttachment->setBlendingState(MTL4::BlendStateEnabled);
			colorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
			colorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
			colorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
			colorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
			colorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
			colorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
		}

		// Metal 4 render pipeline descriptors do not specify depth/stencil
		// attachment pixel formats up front. The render pass provides the
		// actual depth attachment texture. Keep depthEnabled for render-pass
		// setup and depth-state binding, not pipeline descriptor setup.
		(void) depthEnabled;

		NS::Error                *pipelineError = nullptr;
		MTL::RenderPipelineState *newPipeline   = compiler->newRenderPipelineState(
		    pipelineDesc, static_cast<MTL4::CompilerTaskOptions *>(nullptr), &pipelineError);

		pipelineDesc->release();
		fragmentFunction->release();
		constants->release();
		fragmentBaseFunction->release();
		vertexFunction->release();

		if (!newPipeline)
		{
			throw std::runtime_error(
			    ErrorMessage(pipelineError, "Failed to create Metal 4 render pipeline state."));
		}

		return addPipeline(newPipeline, hasBaseColorTexture);
	}
};

BufferHandle MetalRendererBackend::createBuffer(const BufferDesc &desc)
{
	return m_impl->createResidentBuffer(desc.data, desc.size, "Failed to create Metal buffer.");
}

TextureHandle MetalRendererBackend::createTexture(TextureDesc &desc)
{
	stbi_uc *pixels =
	    stbi_load(desc.filename, &desc.width, &desc.height, &desc.channels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error(stbi_failure_reason());
	}

	MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
	textureDescriptor->setTextureType(MTL::TextureType2D);
	textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
	textureDescriptor->setWidth(static_cast<NS::UInteger>(desc.width));
	textureDescriptor->setHeight(static_cast<NS::UInteger>(desc.height));
	textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
	textureDescriptor->setStorageMode(MTL::StorageModeShared);
	textureDescriptor->setMipmapLevelCount(1);

	MTL::Texture *texture = m_impl->device->newTexture(textureDescriptor);
	textureDescriptor->release();

	if (!texture)
	{
		stbi_image_free(pixels);
		throw std::runtime_error("Failed to create Metal texture.");
	}

	const MTL::Region region(0, 0, 0, static_cast<NS::UInteger>(desc.width),
	                         static_cast<NS::UInteger>(desc.height), 1);

	const NS::UInteger bytesPerRow = static_cast<NS::UInteger>(4 * desc.width);
	texture->replaceRegion(region, 0, pixels, bytesPerRow);

	stbi_image_free(pixels);

	m_impl->residencySet->addAllocation(texture);
	m_impl->residencySet->commit();

	return m_impl->addTexture(texture);
}

PipelineHandle MetalRendererBackend::createPipeline(const PipelineDesc &desc)
{
	if (!desc.vertexFunction || !desc.fragmentFunction)
	{
		throw std::runtime_error("PipelineDesc requires vertex and fragment function names.");
	}

	return m_impl->createPipeline(desc);
}

MetalRendererBackend::MetalRendererBackend(Anjean::Window &window) : m_impl(std::make_unique<Impl>())
{
	NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();

	m_impl->device = MTL::CreateSystemDefaultDevice();

	if (!m_impl->device)
	{
		pool->release();
		throw std::runtime_error("Metal is not supported on this system.");
	}

	m_impl->metalView = SDL_Metal_CreateView(window.getNativeWindow());

	if (!m_impl->metalView)
	{
		m_impl->destroy();
		pool->release();
		throw std::runtime_error("Failed to create SDL Metal view.");
	}

	m_impl->layer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(m_impl->metalView));

	if (!m_impl->layer)
	{
		m_impl->destroy();
		pool->release();
		throw std::runtime_error("Failed to get CAMetalLayer.");
	}

	m_impl->layer->setDevice(m_impl->device);
	m_impl->layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
	m_impl->layer->setFramebufferOnly(true);
	m_impl->layer->setDisplaySyncEnabled(false);
	m_impl->layer->setMaximumDrawableCount(3);

	m_impl->commandQueue = m_impl->device->newMTL4CommandQueue();

	if (!m_impl->commandQueue)
	{
		m_impl->destroy();
		pool->release();
		throw std::runtime_error("Failed to create Metal 4 command queue.");
	}

	auto      *residencyDesc  = MTL::ResidencySetDescriptor::alloc()->init();
	NS::Error *residencyError = nullptr;

	m_impl->residencySet = m_impl->device->newResidencySet(residencyDesc, &residencyError);
	residencyDesc->release();

	if (!m_impl->residencySet)
	{
		const char *message = ErrorMessage(residencyError, "Failed to create Metal residency set.");
		m_impl->destroy();
		pool->release();
		throw std::runtime_error(message);
	}

	m_impl->commandQueue->addResidencySet(m_impl->residencySet);

	if (m_impl->layer->residencySet())
	{
		m_impl->commandQueue->addResidencySet(m_impl->layer->residencySet());
	}

	m_impl->createDepthState();
	m_impl->createDefaultSampler();
	m_impl->createDefaultBuffers();

	m_impl->commandBuffer = m_impl->device->newCommandBuffer();

	if (!m_impl->commandBuffer)
	{
		m_impl->destroy();
		pool->release();
		throw std::runtime_error("Failed to create Metal 4 command buffer.");
	}

	m_impl->commandAllocator = m_impl->device->newCommandAllocator();

	if (!m_impl->commandAllocator)
	{
		m_impl->destroy();
		pool->release();
		throw std::runtime_error("Failed to create Metal 4 command allocator.");
	}

	pool->release();
}

MetalRendererBackend::~MetalRendererBackend()
{
	if (m_impl)
	{
		m_impl->destroy();
	}
}

void MetalRendererBackend::beginFrame(const Color &clearColor)
{
	if (m_impl->renderEncoder || m_impl->drawable)
	{
		throw std::runtime_error("beginFrame() called before endFrame().");
	}

	NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();

	m_impl->drawable = m_impl->layer->nextDrawable();

	if (!m_impl->drawable)
	{
		pool->release();
		return;
	}

	m_impl->drawable->retain();
	m_impl->resetFrameArgumentTables();

	MTL::Texture *drawableTexture = m_impl->drawable->texture();
	m_impl->ensureDepthTexture(static_cast<std::uint32_t>(drawableTexture->width()),
	                           static_cast<std::uint32_t>(drawableTexture->height()));

	m_impl->commandBuffer->beginCommandBuffer(m_impl->commandAllocator);

	auto *passDesc = MTL4::RenderPassDescriptor::alloc()->init();

	auto *colorAttachment = passDesc->colorAttachments()->object(0);
	colorAttachment->setTexture(drawableTexture);
	colorAttachment->setLoadAction(MTL::LoadActionClear);
	colorAttachment->setStoreAction(MTL::StoreActionStore);
	colorAttachment->setClearColor(
	    MTL::ClearColor::Make(clearColor.r, clearColor.g, clearColor.b, clearColor.a));

	auto *depthAttachment = passDesc->depthAttachment();
	depthAttachment->setTexture(m_impl->depthTexture);
	depthAttachment->setLoadAction(MTL::LoadActionClear);
	depthAttachment->setStoreAction(MTL::StoreActionDontCare);
	depthAttachment->setClearDepth(1.0);

	m_impl->renderEncoder = m_impl->commandBuffer->renderCommandEncoder(passDesc);
	passDesc->release();

	if (!m_impl->renderEncoder)
	{
		m_impl->releaseFrameObjects();
		pool->release();
		throw std::runtime_error("Failed to create Metal 4 render command encoder.");
	}

	m_impl->renderEncoder->retain();

	pool->release();
}

std::pair<decltype(BufferHandle::id), std::optional<decltype(TextureHandle::id)>>
    MetalRendererBackend::loadModelToGPU(Mesh pMesh)
{
	if (pMesh.vertices.empty())
	{
		throw std::runtime_error("Cannot upload a mesh with no vertices.");
	}

	BufferDesc desc;
	desc.data = pMesh.vertices.data();
	desc.size = pMesh.vertices.size() * sizeof(pMesh.vertices[0]);

	Anjean::Rendering::Mesh mesh;
	mesh.vertexBuffer = createBuffer(desc);
	mesh.vertexCount  = pMesh.vertexCount != 0 ? pMesh.vertexCount : static_cast<decltype(mesh.vertexCount)>(pMesh.vertices.size());

	std::optional<decltype(TextureHandle::id)> textureId = std::nullopt;

	if (pMesh.textureDesc.has_value())
	{
		TextureDesc  &textureDesc   = pMesh.textureDesc.value();
		TextureHandle textureHandle = createTexture(textureDesc);
		textureId                   = textureHandle.id;
	}

	return {mesh.vertexBuffer.id, textureId};
}

void MetalRendererBackend::draw(const DrawCommand &command)
{
	if (!m_impl->renderEncoder)
	{
		return;
	}

	MTL::RenderPipelineState *pipeline     = m_impl->getPipeline(command.pipeline);
	MTL::Buffer              *vertexBuffer = m_impl->getBuffer(command.mesh.vertexBuffer);

	MTL4::ArgumentTable *vertexTable = m_impl->acquireVertexArgumentTable();
	vertexTable->setAddress(vertexBuffer->gpuAddress(), 0);

	m_impl->renderEncoder->setRenderPipelineState(pipeline);
	m_impl->renderEncoder->setCullMode(MTL::CullModeNone);
	m_impl->renderEncoder->setDepthStencilState(m_impl->depthState);
	m_impl->renderEncoder->setArgumentTable(vertexTable, MTL::RenderStageVertex);

	m_impl->renderEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0,
	                                      static_cast<NS::UInteger>(command.mesh.vertexCount), 1);
}

void MetalRendererBackend::drawSprite(const PipelineHandle               &pPipeline,
                                      const Core::MeshData               &pMesh,
                                      const std::optional<TextureHandle> &pTexture,
                                      const ObjectUniformHandle          &pObjectUniform)
{
	if (!m_impl->renderEncoder)
	{
		return;
	}

	MTL::RenderPipelineState *pipeline     = m_impl->getPipeline(pPipeline);
	MTL::Buffer              *vertexBuffer = m_impl->getBuffer(BufferHandle{pMesh.id});
	MTL::Buffer              *objectUniformBuffer =
	    m_impl->createFrameUploadBuffer(&pObjectUniform, sizeof(ObjectUniformHandle));
	MTL::Buffer *frameUniformBuffer    = m_impl->getBuffer(m_impl->frameUniformBuffer);
	MTL::Buffer *materialUniformBuffer = m_impl->getBuffer(m_impl->defaultMaterialUniformBuffer);

	const bool pipelineNeedsTexture = m_impl->pipelineRequiresBaseColorTexture(pPipeline);

	if (pipelineNeedsTexture && !pTexture.has_value())
	{
		throw std::runtime_error("Textured pipeline was used without a base color texture.");
	}

	// New baseline binding:
	//   buffer(1) = ObjectUniforms { float4x4 model; }
	//   buffer(2) = FrameUniforms  { float4x4 viewProjection; }
	//
	// Compatibility path for the old uploaded shader layout:
	//   ObjectUniforms { model, modelTranslation, modelScale, modelRot, viewProjection }
	// If the supplied object buffer is large enough to contain that old layout, bind
	// buffer(2) directly to the viewProjection matrix inside the same buffer. This
	// preserves existing 3D camera behavior while letting the new shader use split
	// object/frame bindings. New code should eventually update frameUniformBuffer
	// once per camera/pass instead.
	constexpr NS::UInteger kFloat4x4ByteSize             = sizeof(float) * 16;
	constexpr NS::UInteger kLegacyViewProjectionOffset   = kFloat4x4ByteSize * 4;
	constexpr NS::UInteger kLegacyObjectUniformsByteSize = kFloat4x4ByteSize * 5;

	auto objectUniformAddress = objectUniformBuffer->gpuAddress();
	auto frameUniformAddress  = frameUniformBuffer->gpuAddress();

	if (objectUniformBuffer->length() >= kLegacyObjectUniformsByteSize)
	{
		frameUniformAddress = objectUniformAddress + kLegacyViewProjectionOffset;
	}

	MTL4::ArgumentTable *vertexTable = m_impl->acquireVertexArgumentTable();
	vertexTable->setAddress(vertexBuffer->gpuAddress(), 0);
	vertexTable->setAddress(objectUniformAddress, 1);
	vertexTable->setAddress(frameUniformAddress, 2);

	MTL4::ArgumentTable *fragmentTable = m_impl->acquireFragmentArgumentTable();
	fragmentTable->setAddress(materialUniformBuffer->gpuAddress(), 0);
	fragmentTable->setSamplerState(m_impl->nearestSampler->gpuResourceID(), 0);

	if (pTexture.has_value())
	{
		MTL::Texture *texture = m_impl->getTexture(pTexture.value());
		fragmentTable->setTexture(texture->gpuResourceID(), 0);
	}

	m_impl->renderEncoder->setRenderPipelineState(pipeline);
	m_impl->renderEncoder->setCullMode(MTL::CullModeNone);
	m_impl->renderEncoder->setDepthStencilState(m_impl->depthState);
	m_impl->renderEncoder->setArgumentTable(vertexTable, MTL::RenderStageVertex);
	m_impl->renderEncoder->setArgumentTable(fragmentTable, MTL::RenderStageFragment);

	m_impl->renderEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0,
	                                      static_cast<NS::UInteger>(pMesh.vertices.size()), 1);
}

void MetalRendererBackend::endFrame()
{
	if (!m_impl->renderEncoder || !m_impl->drawable)
	{
		return;
	}

	m_impl->renderEncoder->endEncoding();
	m_impl->commandBuffer->endCommandBuffer();

	auto *drawable = reinterpret_cast<MTL::Drawable *>(m_impl->drawable);

	m_impl->commandQueue->wait(drawable);
	m_impl->commandQueue->commit(&m_impl->commandBuffer, 1);
	m_impl->commandQueue->signalDrawable(drawable);
	drawable->present();

	m_impl->releaseFrameObjects();
}
}        // namespace Anjean::Rendering
