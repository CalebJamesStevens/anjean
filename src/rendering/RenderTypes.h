#pragma once

#include <cstddef>
#include <cstdint>
#include <simd/simd.h>

#include "../Core/Core.h"

namespace Anjean::Rendering {
  struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
  };

  struct ObjectUniform {
    simd_float4x4 model;
    simd_float4x4 modelTranslation;
    simd_float4x4 modelScale;
    simd_float4x4 modelRot;
    simd_float4x4 viewProjection;
  };

  struct BufferHandle {
    std::uint32_t id = 0;
  };

  struct TextureHandle {
    std::uint32_t id = 0;
  };

  struct PipelineHandle {
    std::uint32_t id = 0;
  };

  struct Vertex2D {
    float position[4];
    float color[4];
  };

  struct TexturedVertex2D {
    float position[2];
    float textureCoordinate[2];
  };

  struct Vertex3D {
    float position[3];
    float textureCoordinate[2];
  };

  struct TexturedVertex3D {
    float position[3];
    float textureCoordinate[2];
  };

  struct BufferDesc {
    const void* data = nullptr;
    std::size_t size = 0;
  };

  struct TextureDesc {
    int width = 0;
    int height = 0;
    int channels = 0;
    const char* filename = nullptr;
    std::size_t size = 0;
  };

  struct Mesh {
    BufferHandle vertexBuffer;
    std::uint32_t vertexCount = 0;
    std::vector<Anjean::Core::MeshVertex> vertices;
    std::optional<TextureDesc> textureDesc;
  };

  struct PipelineDesc {
    const char* vertexFunction = nullptr;
    const char* fragmentFunction = nullptr;
    const char* debugName = "Unnamed Pipeline";
  };

  struct DrawCommand {
    PipelineHandle pipeline;
    Mesh mesh;
  };

  enum class GraphicsAPI { Auto, Metal, Vulkan };

  struct RendererConfig {
    GraphicsAPI api = GraphicsAPI::Auto;
  };
} // namespace Anjean::Rendering