#include "MetalRendererBackend.h"
#include "../../../window/Window.h"

#include <SDL3/SDL_metal.h>

#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <Metal/Metal.hpp>

#include <stdexcept>

namespace Anjean
{
    static NS::String* MakeNSString(const char* text)
    {
        return NS::String::string(text, NS::UTF8StringEncoding);
    }

    struct MetalRendererBackend::Impl
    {
        SDL_MetalView metalView = nullptr;

        CA::MetalLayer* layer = nullptr;
        MTL::Device* device = nullptr;

        MTL4::CommandQueue* commandQueue = nullptr;
        MTL4::CommandBuffer* commandBuffer = nullptr;
        MTL4::CommandAllocator* commandAllocator = nullptr;

        CA::MetalDrawable* drawable = nullptr;
        MTL4::RenderCommandEncoder* renderEncoder = nullptr;

        MTL::Library* shaderLibrary = nullptr;
        MTL4::Compiler* compiler = nullptr;
        MTL::RenderPipelineState* pipelineState = nullptr;

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

            if (compiler)
            {
                compiler->release();
                compiler = nullptr;
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
                layer = nullptr;
            }

            if (device)
            {
                device->release();
                device = nullptr;
            }
        }

        void createPipeline()
        {
            shaderLibrary = device->newLibrary(
                MakeNSString(ANJEAN_METAL_SHADER_LIBRARY),
                nullptr
            );

            if (!shaderLibrary)
            {
                throw std::runtime_error("Failed to load Metal shader library.");
            }

            auto* compilerDesc = MTL4::CompilerDescriptor::alloc()->init();
            compiler = device->newCompiler(compilerDesc, nullptr);
            compilerDesc->release();

            if (!compiler)
            {
                throw std::runtime_error("Failed to create Metal 4 compiler.");
            }

            auto* vertexFunction = MTL4::LibraryFunctionDescriptor::alloc()->init();
            vertexFunction->setLibrary(shaderLibrary);
            vertexFunction->setName(MakeNSString("vertex_shader"));

            auto* fragmentFunction = MTL4::LibraryFunctionDescriptor::alloc()->init();
            fragmentFunction->setLibrary(shaderLibrary);
            fragmentFunction->setName(MakeNSString("fragment_shader"));

            auto* pipelineDesc = MTL4::RenderPipelineDescriptor::alloc()->init();
            pipelineDesc->setLabel(MakeNSString("Anjean Test Triangle Pipeline"));
            pipelineDesc->setVertexFunctionDescriptor(vertexFunction);
            pipelineDesc->setFragmentFunctionDescriptor(fragmentFunction);

            pipelineDesc
                ->colorAttachments()
                ->object(0)
                ->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

            pipelineState = compiler->newRenderPipelineState(
                pipelineDesc,
                static_cast<MTL4::CompilerTaskOptions*>(nullptr),
                static_cast<NS::Error**>(nullptr)
            );

            pipelineDesc->release();
            fragmentFunction->release();
            vertexFunction->release();

            if (!pipelineState)
            {
                throw std::runtime_error("Failed to create Metal 4 render pipeline state.");
            }
        }
    };

    MetalRendererBackend::MetalRendererBackend(Window& window)
        : m_impl(std::make_unique<Impl>())
    {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

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

        m_impl->layer =
            static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(m_impl->metalView));

        if (!m_impl->layer)
        {
            m_impl->destroy();
            pool->release();
            throw std::runtime_error("Failed to get CAMetalLayer.");
        }

        m_impl->layer->setDevice(m_impl->device);
        m_impl->layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
        m_impl->layer->setFramebufferOnly(true);

        m_impl->commandQueue = m_impl->device->newMTL4CommandQueue();

        if (!m_impl->commandQueue)
        {
            m_impl->destroy();
            pool->release();
            throw std::runtime_error("Failed to create Metal 4 command queue.");
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

        m_impl->createPipeline();

        pool->release();
    }

    MetalRendererBackend::~MetalRendererBackend()
    {
        if (m_impl)
        {
            m_impl->destroy();
        }
    }

    void MetalRendererBackend::beginFrame(const Color& clearColor)
    {
        if (m_impl->renderEncoder || m_impl->drawable)
        {
            throw std::runtime_error("beginFrame() called before endFrame().");
        }

        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

        m_impl->drawable = m_impl->layer->nextDrawable();

        if (!m_impl->drawable)
        {
            pool->release();
            return;
        }

        m_impl->drawable->retain();

        m_impl->commandBuffer->beginCommandBuffer(m_impl->commandAllocator);

        auto* passDesc = MTL4::RenderPassDescriptor::alloc()->init();

        auto* colorAttachment =
            passDesc->colorAttachments()->object(0);

        colorAttachment->setTexture(m_impl->drawable->texture());
        colorAttachment->setLoadAction(MTL::LoadActionClear);
        colorAttachment->setStoreAction(MTL::StoreActionStore);
        colorAttachment->setClearColor(
            MTL::ClearColor::Make(
                clearColor.r,
                clearColor.g,
                clearColor.b,
                clearColor.a
            )
        );

        m_impl->renderEncoder =
            m_impl->commandBuffer->renderCommandEncoder(passDesc);

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

    void MetalRendererBackend::drawTestTriangle()
    {
        if (!m_impl->renderEncoder || !m_impl->pipelineState)
        {
            return;
        }

        m_impl->renderEncoder->setRenderPipelineState(m_impl->pipelineState);
        m_impl->renderEncoder->drawPrimitives(
            MTL::PrimitiveTypeTriangle,
            0,
            3
        );
    }

    void MetalRendererBackend::endFrame()
    {
        if (!m_impl->renderEncoder || !m_impl->drawable)
        {
            return;
        }

        m_impl->renderEncoder->endEncoding();
        m_impl->commandBuffer->endCommandBuffer();

        auto* drawable = reinterpret_cast<MTL::Drawable*>(m_impl->drawable);

        m_impl->commandQueue->wait(drawable);
        m_impl->commandQueue->commit(&m_impl->commandBuffer, 1);
        m_impl->commandQueue->signalDrawable(drawable);
        drawable->present();

        m_impl->releaseFrameObjects();
    }
}