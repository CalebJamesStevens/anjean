#pragma once

#include "RenderTypes.h"

namespace Anjean::Rendering
{
class IRenderBackend
{
  public:
	virtual ~IRenderBackend() = default;

	virtual void           beginFrame(const Color &clearColor)                                                                                                       = 0;
	virtual void           onResize(int width, int height)                                                                                                           = 0;
	virtual BufferHandle   createBuffer(const BufferDesc &desc)                                                                                                      = 0;
	virtual TextureHandle  createTexture(TextureDesc &desc)                                                                                                          = 0;
	virtual PipelineHandle createPipeline(const PipelineDesc &desc)                                                                                                  = 0;
	virtual void           drawSprite(const PipelineHandle &pPipeline, const Core::MeshData &pMesh,
	                                  const std::optional<TextureHandle> &pTexture,
	                                  const ObjectUniform                &pObjectUniform)                                                                            = 0;
	virtual void           renderFrame(const Anjean::Core::CameraPacket &cameraPacket, const Color &clearColor, std::span<const Anjean::Core::RenderPacket> packets) = 0;
	virtual void           loadModelToGPU(std::span<const Core::ImportedModel *const> models)                                                                        = 0;

	virtual void endFrame() = 0;
};
}        // namespace Anjean::Rendering