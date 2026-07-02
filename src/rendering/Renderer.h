#pragma once

#include <memory>

#include "../runtime/RuntimeTypes.h"
#include "../window/Window.h"
#include "RenderTypes.h"

namespace Anjean::Rendering
{
class IRenderBackend;

class Renderer
{
  public:
	explicit Renderer(Anjean::Window &window, const RendererConfig &config = {});
	~Renderer();

	Renderer(const Renderer &)            = delete;
	Renderer &operator=(const Renderer &) = delete;

	Renderer(Renderer &&) noexcept            = default;
	Renderer &operator=(Renderer &&) noexcept = default;

	void           beginFrame(const Color &clearColor);
	TextureHandle  createTexture(TextureDesc &desc);
	BufferHandle   createBuffer(const BufferDesc &desc);
	PipelineHandle createPipeline(const PipelineDesc &desc);
	PipelineHandle createSpritePipeline();
	std::pair<decltype(BufferHandle::id), std::optional<decltype(TextureHandle::id)>>
	     loadMeshToGPU(Anjean::Core::MeshData pMesh);
	void drawSprite(const PipelineHandle &pPipeline, const Core::MeshData &pMesh,
	                const std::optional<TextureHandle> &pTexture,
	                const ObjectUniform                &pObjectUniform);
	void endFrame();
	void onResize(int width, int height);
	void renderFrame(const Anjean::Core::CameraPacket &cameraPacket, const Color &clearColor, std::span<const Anjean::Core::RenderPacket> packets);

  private:
	std::unique_ptr<IRenderBackend> m_backend;
};
}        // namespace Anjean::Rendering