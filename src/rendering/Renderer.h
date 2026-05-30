#pragma once

#include <memory>

#include "RenderTypes.h"

namespace Anjean
{
    class Window;
    class IRenderBackend;

    class Renderer
    {
    public:
        explicit Renderer(Window& window, const RendererConfig& config = {});
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        Renderer(Renderer&&) noexcept = default;
        Renderer& operator=(Renderer&&) noexcept = default;

        void beginFrame(const Color& clearColor);
        TextureHandle createTexture(TextureDesc& desc);
        BufferHandle createBuffer(const BufferDesc& desc);
        PipelineHandle createPipeline(const PipelineDesc& desc);
        void draw(const DrawCommand& command);
        void drawSprite(const PipelineHandle& pPipeline, const Mesh& pMesh, const TextureHandle& pTexture, const ObjectUniformHandle& pObjectUniform);
        void endFrame();

    private:
        std::unique_ptr<IRenderBackend> m_backend;
    };
}