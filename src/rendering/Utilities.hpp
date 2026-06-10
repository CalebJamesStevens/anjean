#pragma once

#include <simd/simd.h>

namespace Anjean::Rendering
{
    simd_float4x4 makeRotationX(float radians);
    simd_float4x4 makeRotationY(float radians);
    simd_float4x4 makeRotationZ(float radians);
    simd_float4x4 makePerspective(float fovYRadians, float aspect, float nearZ, float farZ);
}