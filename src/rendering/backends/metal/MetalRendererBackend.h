#pragma once

#include "../../../runtime/Runtime.h"
#include "../../../window/Window.h"
#include "../../IRenderBackend.h"

#include <memory>

namespace Anjean::Rendering
{
struct MetalVertex2D
{
	float position[2];
	float textureCoordinate[2];
};

class MetalRendererBackend final : public IRenderBackend
{
  public:
	explicit MetalRendererBackend(Anjean::Window &window);
	~MetalRendererBackend() override;

	MetalRendererBackend(const MetalRendererBackend &)            = delete;
	MetalRendererBackend &operator=(const MetalRendererBackend &) = delete;

	MetalRendererBackend(MetalRendererBackend &&) noexcept            = delete;
	MetalRendererBackend &operator=(MetalRendererBackend &&) noexcept = delete;

	void beginFrame(const Color &clearColor) override;

	BufferHandle   createBuffer(const BufferDesc &desc) override;
	TextureHandle  createTexture(TextureDesc &desc) override;
	PipelineHandle createPipeline(const PipelineDesc &desc) override;
	void           drawSprite(const PipelineHandle &pPipeline, const Core::MeshData &pMesh,
	                          const std::optional<TextureHandle> &pTexture,
	                          const ObjectUniform                &pObjectUniform) override;
	std::pair<decltype(BufferHandle::id), std::optional<decltype(TextureHandle::id)>>
	     loadModelToGPU(Anjean::Core::MeshData pMesh) override;
	void endFrame() override;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
}        // namespace Anjean::Rendering