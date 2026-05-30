#include <metal_stdlib>
using namespace metal;

struct Vertex2D
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
    uint vertexId [[vertex_id]],
    const device Vertex2D* vertices [[buffer(0)]]
)
{
    Vertex2D v = vertices[vertexId];

    VertexOut out;
    out.position = v.position;
    out.color = v.color;

    return out;
}

fragment float4 fragment_shader(VertexOut in [[stage_in]])
{
    return in.color;
}