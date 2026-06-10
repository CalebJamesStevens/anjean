#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <simd/simd.h>

#include "Utilities.hpp"

namespace Anjean::Rendering {
  simd_float4x4 makeRotationX(float radians)
  {
      float c = std::cos(radians);
      float s = std::sin(radians);

      return simd_float4x4{
          simd_make_float4(1,  0,  0, 0),
          simd_make_float4(0,  c,  s, 0),
          simd_make_float4(0, -s,  c, 0),
          simd_make_float4(0,  0,  0, 1)
      };
  }

  simd_float4x4 makeRotationY(float radians)
  {
      float c = std::cos(radians);
      float s = std::sin(radians);

      return simd_float4x4{
          simd_make_float4( c, 0, -s, 0),
          simd_make_float4( 0, 1,  0, 0),
          simd_make_float4( s, 0,  c, 0),
          simd_make_float4( 0, 0,  0, 1)
      };
  }

  simd_float4x4 makeRotationZ(float radians)
  {
      float c = std::cos(radians);
      float s = std::sin(radians);

      return simd_float4x4{
          simd_make_float4( c,  s, 0, 0),
          simd_make_float4(-s,  c, 0, 0),
          simd_make_float4( 0,  0, 1, 0),
          simd_make_float4( 0,  0, 0, 1)
      };
  }
  simd_float4x4 makePerspective(
    float fovYRadians,
    float aspect,
    float nearZ,
    float farZ
  )
  {
      float yScale = 1.0f / tanf(fovYRadians * 0.5f);
      float xScale = yScale / aspect;
      float zRange = nearZ - farZ;

      return simd_float4x4{
          simd_make_float4(xScale, 0,      0,                      0),
          simd_make_float4(0,      yScale, 0,                      0),
          simd_make_float4(0,      0,      farZ / zRange,         -1),
          simd_make_float4(0,      0,      nearZ * farZ / zRange,  0)
      };
  }
}