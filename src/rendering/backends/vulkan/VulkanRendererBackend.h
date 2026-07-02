#pragma once

#include "../../../runtime/Runtime.h"
#include "../../../window/Window.h"
#include "../../IRenderBackend.h"

#include <memory>

namespace Anjean::Rendering
{
struct VulkanVertex2D
{
	float position[2];
	float textureCoordinate[2];
};

class VulkanRendererBackend final : public IRenderBackend
{
  public:
	explicit VulkanRendererBackend(Anjean::Window &window);
	~VulkanRendererBackend() override;

	VulkanRendererBackend(const VulkanRendererBackend &)            = delete;
	VulkanRendererBackend &operator=(const VulkanRendererBackend &) = delete;

	VulkanRendererBackend(VulkanRendererBackend &&) noexcept            = delete;
	VulkanRendererBackend &operator=(VulkanRendererBackend &&) noexcept = delete;

	void beginFrame(const Color &clearColor) override;

	BufferHandle   createBuffer(const BufferDesc &desc) override;
	TextureHandle  createTexture(TextureDesc &desc) override;
	PipelineHandle createPipeline(const PipelineDesc &desc) override;
	void           drawSprite(const PipelineHandle &pPipeline, const Core::MeshData &pMesh,
	                          const std::optional<TextureHandle> &pTexture,
	                          const ObjectUniform                &pObjectUniform) override;
	std::pair<decltype(BufferHandle::id), std::optional<decltype(TextureHandle::id)>>
	     loadMeshToGPU(Anjean::Core::MeshData pMesh) override;
	void endFrame() override;
	void onResize(int width, int height) override;
	void renderFrame(const Anjean::Core::CameraPacket &cameraPacket, const Color &clearColor, std::span<const Anjean::Core::RenderPacket> packets) override;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
}        // namespace Anjean::Rendering