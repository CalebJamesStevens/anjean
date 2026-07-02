#include "MetalRendererBackend.h"
#include "../../../runtime/RuntimeTypes.h"
#include "../../../window/Window.h"

#include <SDL3/SDL_metal.h>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <iostream>
#include <stb_image.h>

#include <stdexcept>

namespace Anjean::Rendering
{
static NS::String *MakeNSString(const char *text)
{
	return NS::String::string(text, NS::UTF8StringEncoding);
}

static constexpr std::size_t alignUp(std::size_t value, std::size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

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

	static constexpr std::uint32_t kFramesInFlight     = 3;
	static constexpr std::size_t   kMaxSpritesPerFrame = 8192;

	static constexpr std::size_t kObjectUniformAlignment = 256;
	static constexpr std::size_t kObjectUniformStride =
	    alignUp(sizeof(ObjectUniform), kObjectUniformAlignment);

	std::array<MTL::Buffer *, kFramesInFlight> objectUniformBuffers{};
	std::uint32_t                              frameIndex               = 0;
	std::size_t                                objectUniformWriteOffset = 0;

	MTL::Library                           *shaderLibrary = nullptr;
	MTL4::Compiler                         *compiler      = nullptr;
	MTL::RenderPipelineState               *pipelineState = nullptr;
	std::vector<MTL::Buffer *>              buffers;
	std::vector<MTL::Texture *>             textures;
	std::vector<MTL::RenderPipelineState *> pipelines;
	MTL::ResidencySet                      *residencySet = nullptr;
	MTL::Texture                           *depthTexture = nullptr;
	MTL::DepthStencilState                 *depthState   = nullptr;

	BufferHandle addBuffer(MTL::Buffer *buffer)
	{
		buffers.push_back(buffer);
		return BufferHandle{static_cast<std::uint32_t>(buffers.size())};
	}

	TextureHandle addTexture(MTL::Texture *texture)
	{
		textures.push_back(texture);
		return TextureHandle{static_cast<std::uint32_t>(textures.size())};
	}

	PipelineHandle addPipeline(MTL::RenderPipelineState *pipeline)
	{
		pipelines.push_back(pipeline);
		return PipelineHandle{static_cast<std::uint32_t>(pipelines.size())};
	}

	MTL::Buffer *getBuffer(BufferHandle handle)
	{
		return buffers.at(handle.id - 1);
	}

	MTL::Texture *getTexture(TextureHandle handle)
	{
		return textures.at(handle.id - 1);
	}

	MTL::RenderPipelineState *getPipeline(PipelineHandle handle)
	{
		return pipelines.at(handle.id - 1);
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
	}

	void destroy()
	{
		releaseFrameObjects();

		if (pipelineState)
		{
			pipelineState->release();
			pipelineState = nullptr;
		}

		for (auto *pipeline : pipelines)
		{
			if (pipeline)
			{
				pipeline->release();
			}
		}

		pipelines.clear();

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

		if (depthState)
		{
			depthState->release();
			depthState = nullptr;
		}

		if (depthTexture)
		{
			depthTexture->release();
			depthTexture = nullptr;
		}

		if (compiler)
		{
			compiler->release();
			compiler = nullptr;
		}

		for (auto *&buffer : objectUniformBuffers)
		{
			if (buffer)
			{
				// If your Metal 4 residency API supports it, remove first:
				// residencySet->removeAllocation(buffer);

				buffer->release();
				buffer = nullptr;
			}
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

		compiler = device->newCompiler(compilerDesc, nullptr);

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

		auto *vertexFunction = MTL4::LibraryFunctionDescriptor::alloc()->init();
		vertexFunction->setLibrary(shaderLibrary);
		vertexFunction->setName(MakeNSString(desc.vertexFunction));

		auto *fragmentFunction = MTL4::LibraryFunctionDescriptor::alloc()->init();
		fragmentFunction->setLibrary(shaderLibrary);
		fragmentFunction->setName(MakeNSString(desc.fragmentFunction));

		auto *pipelineDesc = MTL4::RenderPipelineDescriptor::alloc()->init();
		pipelineDesc->setLabel(MakeNSString(desc.debugName));
		pipelineDesc->setVertexFunctionDescriptor(vertexFunction);
		pipelineDesc->setFragmentFunctionDescriptor(fragmentFunction);
		auto *colorAttachment = pipelineDesc->colorAttachments()->object(0);

		colorAttachment->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
		colorAttachment->setBlendingState(MTL4::BlendStateEnabled);

		colorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
		colorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);

		colorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
		colorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

		colorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
		colorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

		pipelineDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

		MTL::RenderPipelineState *newPipeline = compiler->newRenderPipelineState(
		    pipelineDesc, static_cast<MTL4::CompilerTaskOptions *>(nullptr),
		    static_cast<NS::Error **>(nullptr));

		pipelineDesc->release();
		fragmentFunction->release();
		vertexFunction->release();

		if (!newPipeline)
		{
			throw std::runtime_error("Failed to create Metal 4 render pipeline state.");
		}

		residencySet->addAllocation(newPipeline);
		residencySet->commit();

		return addPipeline(newPipeline);
	}
};

BufferHandle MetalRendererBackend::createBuffer(const BufferDesc &desc)
{
	if (!desc.data || desc.size == 0)
	{
		throw std::runtime_error("Cannot create buffer with empty data.");
	}

	MTL::Buffer *buffer =
	    m_impl->device->newBuffer(desc.data, desc.size, MTL::ResourceStorageModeShared);

	if (!buffer)
	{
		throw std::runtime_error("Failed to create Metal buffer.");
	}

	m_impl->residencySet->addAllocation(buffer);
	m_impl->residencySet->commit();

	return m_impl->addBuffer(buffer);
}

TextureHandle MetalRendererBackend::createTexture(TextureDesc &desc)
{
	stbi_uc *pixels =
	    stbi_load(desc.filename, &(desc.width), &(desc.height), &desc.channels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error(stbi_failure_reason());
	}

	MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
	textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
	textureDescriptor->setWidth(desc.width);
	textureDescriptor->setHeight(desc.height);
	textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
	textureDescriptor->setStorageMode(MTL::StorageModeShared);

	MTL::Texture *texture = m_impl->device->newTexture(textureDescriptor);
	textureDescriptor->release();

	if (!texture)
	{
		stbi_image_free(pixels);
		throw std::runtime_error("Failed to create Metal texture.");
	}

	m_impl->residencySet->addAllocation(texture);
	m_impl->residencySet->commit();

	MTL::Origin textureOrigin(0, 0, 0);
	MTL::Size   textureSize(static_cast<NS::UInteger>(desc.width),
	                        static_cast<NS::UInteger>(desc.height), 1);

	MTL::Region region(textureOrigin.x, textureOrigin.y, textureOrigin.z, textureSize.width,
	                   textureSize.height, textureSize.depth);

	NS::UInteger bytesPerRow = static_cast<NS::UInteger>(4 * (desc.width));
	texture->replaceRegion(region, 0, pixels, bytesPerRow);

	stbi_image_free(pixels);

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

	auto *depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
	depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
	depthDesc->setDepthWriteEnabled(true);

	m_impl->depthState = m_impl->device->newDepthStencilState(depthDesc);

	depthDesc->release();

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
	m_impl->layer->setDrawableSize(CGSize{window.width, window.height});

	m_impl->commandQueue = m_impl->device->newMTL4CommandQueue();

	if (!m_impl->commandQueue)
	{
		m_impl->destroy();
		pool->release();
		throw std::runtime_error("Failed to create Metal 4 command queue.");
	}

	auto *depthTexDesc = MTL::TextureDescriptor::alloc()->init();
	auto  drawableSize = m_impl->layer->drawableSize();

	depthTexDesc->setTextureType(MTL::TextureType2D);
	depthTexDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
	depthTexDesc->setWidth(static_cast<NS::UInteger>(drawableSize.width));
	depthTexDesc->setHeight(static_cast<NS::UInteger>(drawableSize.height));
	depthTexDesc->setStorageMode(MTL::StorageModePrivate);
	depthTexDesc->setUsage(MTL::TextureUsageRenderTarget);

	m_impl->depthTexture = m_impl->device->newTexture(depthTexDesc);

	depthTexDesc->release();

	auto *residencyDesc = MTL::ResidencySetDescriptor::alloc()->init();

	NS::Error *residencyError = nullptr;

	m_impl->residencySet = m_impl->device->newResidencySet(residencyDesc, &residencyError);

	residencyDesc->release();

	if (!m_impl->residencySet)
	{
		const char *message = residencyError ? residencyError->localizedDescription()->utf8String() : "Failed to create Metal residency set.";

		m_impl->destroy();
		pool->release();
		throw std::runtime_error(message);
	}

	if (m_impl->depthTexture)
	{
		m_impl->residencySet->addAllocation(m_impl->depthTexture);
	}

	for (auto &buffer : m_impl->objectUniformBuffers)
	{
		buffer = m_impl->device->newBuffer(Impl::kObjectUniformStride * Impl::kMaxSpritesPerFrame,
		                                   MTL::ResourceStorageModeShared);

		if (!buffer)
		{
			m_impl->destroy();
			pool->release();
			throw std::runtime_error("Failed to create object uniform ring buffer.");
		}

		m_impl->residencySet->addAllocation(buffer);
	}

	m_impl->residencySet->commit();
	m_impl->commandQueue->addResidencySet(m_impl->residencySet);

	// Important for the CAMetalLayer drawable textures too.
	if (m_impl->layer->residencySet())
	{
		m_impl->commandQueue->addResidencySet(m_impl->layer->residencySet());
	}

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

	m_impl->commandBuffer->beginCommandBuffer(m_impl->commandAllocator);

	auto *passDesc = MTL4::RenderPassDescriptor::alloc()->init();

	auto *colorAttachment = passDesc->colorAttachments()->object(0);

	colorAttachment->setTexture(m_impl->drawable->texture());
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

	m_impl->frameIndex               = (m_impl->frameIndex + 1) % Impl::kFramesInFlight;
	m_impl->objectUniformWriteOffset = 0;

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
    MetalRendererBackend::loadModelToGPU(Anjean::Core::MeshData pMesh)
{
	Anjean::Rendering::BufferDesc desc;

	desc.data = pMesh.vertices.data();
	desc.size = pMesh.vertices.size() * sizeof(pMesh.vertices[0]);

	Anjean::Rendering::Mesh mesh;
	mesh.vertexBuffer = createBuffer(desc);
	mesh.vertexCount  = pMesh.vertexCount;

	std::optional<decltype(TextureHandle::id)> returnTextureId = std::nullopt;
	if (pMesh.textureDesc.has_value())
	{
		TextureDesc &texDesc       = pMesh.textureDesc.value();
		auto         textureHandle = createTexture(texDesc);
		returnTextureId            = textureHandle.id;
	}

	return {mesh.vertexBuffer.id, returnTextureId};
}

void MetalRendererBackend::drawSprite(const PipelineHandle               &pPipeline,
                                      const Core::MeshData               &pMesh,
                                      const std::optional<TextureHandle> &pTexture,
                                      const ObjectUniform                &pObjectUniform)
{
	if (!m_impl->renderEncoder)
	{
		return;
	}

	MTL::RenderPipelineState *pipeline    = m_impl->getPipeline(pPipeline);
	MTL::Buffer              *vertextBuff = m_impl->getBuffer({pMesh.id});

	MTL::Texture *texture;
	if (pTexture.has_value())
	{
		texture = m_impl->getTexture(pTexture.value());
	}
	else
	{
		texture = nullptr;
	}

	MTL::Buffer *objectUniformBuffer = m_impl->objectUniformBuffers[m_impl->frameIndex];

	const std::size_t offset = m_impl->objectUniformWriteOffset;

	if (offset + Impl::kObjectUniformStride >
	    Impl::kObjectUniformStride * Impl::kMaxSpritesPerFrame)
	{
		throw std::runtime_error("Object uniform ring buffer overflow.");
	}

	auto *dst = static_cast<std::uint8_t *>(objectUniformBuffer->contents()) + offset;
	std::memcpy(dst, &pObjectUniform, sizeof(ObjectUniform));

	m_impl->objectUniformWriteOffset += Impl::kObjectUniformStride;

	auto *vertexTableDesc = MTL4::ArgumentTableDescriptor::alloc()->init();
	vertexTableDesc->setMaxBufferBindCount(2);

	MTL4::ArgumentTable *vertexTable = m_impl->device->newArgumentTable(vertexTableDesc, nullptr);

	vertexTableDesc->release();

	if (!vertexTable)
	{
		throw std::runtime_error("Failed to create vertex argument table.");
	}

	auto *fragmentTableDesc = MTL4::ArgumentTableDescriptor::alloc()->init();
	fragmentTableDesc->setMaxTextureBindCount(1);
	fragmentTableDesc->setMaxBufferBindCount(1);

	MTL4::ArgumentTable *fragmentTable = nullptr;

	if (texture)
	{
		fragmentTable = m_impl->device->newArgumentTable(fragmentTableDesc, nullptr);
	}

	fragmentTableDesc->release();

	if (!fragmentTable && texture)
	{
		vertexTable->release();
		throw std::runtime_error("Failed to create fragment argument table.");
	}

	vertexTable->setAddress(vertextBuff->gpuAddress(), 0);
	vertexTable->setAddress(objectUniformBuffer->gpuAddress() + offset, 1);
	if (texture)
	{
		fragmentTable->setTexture(texture->gpuResourceID(), 0);
	}

	m_impl->renderEncoder->setRenderPipelineState(pipeline);
	m_impl->renderEncoder->setCullMode(MTL::CullModeNone);
	m_impl->renderEncoder->setArgumentTable(vertexTable, MTL::RenderStageVertex);
	if (texture)
	{
		m_impl->renderEncoder->setArgumentTable(fragmentTable, MTL::RenderStageFragment);
	}

	m_impl->renderEncoder->setDepthStencilState(m_impl->depthState);

	m_impl->renderEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, pMesh.vertices.size(), 1);

	vertexTable->release();
	if (fragmentTable)
	{
		fragmentTable->release();
	}
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