#pragma once
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "../Core/Core.h"

namespace Anjean::Runtime
{
  struct Vec3
  {
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
  };
  
  struct Vertex
  {
      Vec3 position;
  };

  struct Transform 
  {
      Core::Vector3 position;
      Core::Vector3 rotation;
      Core::Vector3 scale = { 1.0f, 1.0f, 1.0f };
  };

  enum GameObjectType {
    ANJEAN_GAMEOBJECT,
    ANJEAN_GAMEOBJECT_CAMERA
  };

  struct Mesh
  {
      std::uint32_t id = 0;
      std::uint32_t vertexCount = 0;
      std::vector<Anjean::Core::MeshVertex> vertices;
  };
}