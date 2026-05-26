#include <metal_stdlib>
using namespace metal;

struct VertexIn
{
    float4 position;
    float4 color;
};

struct VertexOut
{
    float4 position [[position]];
    float4 color;
};

vertex VertexOut vertex_shader(
    uint id [[vertex_id]],
    constant VertexIn* vertices [[buffer(0)]]
)
{
    VertexIn v = vertices[id];

    VertexOut out;
    out.position = v.position;
    out.color = v.color;
    return out;
}

fragment float4 fragment_shader(VertexOut in [[stage_in]])
{
    return in.color;
}