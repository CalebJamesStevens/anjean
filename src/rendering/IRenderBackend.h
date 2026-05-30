#pragma once

#include "RenderTypes.h"

namespace Anjean
{
    class IRenderBackend
    {
    public:
        virtual ~IRenderBackend() = default;

        virtual void beginFrame(const Color& clearColor) = 0;

        virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
        virtual TextureHandle createTexture(TextureDesc& desc) = 0;
        virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
        virtual void draw(const DrawCommand& command) = 0;
        virtual void drawSprite(const PipelineHandle& pPipeline, const Mesh& pMesh, const TextureHandle& pTexture, const ObjectUniformHandle& pObjectUniform) = 0;

        virtual void endFrame() = 0;
    };
}