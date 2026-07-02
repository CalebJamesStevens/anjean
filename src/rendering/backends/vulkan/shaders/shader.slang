struct PrimitiveData
{
    uint vertexOffset;
    uint vertexCount;
    uint indexOffset;
    uint indexCount;
};

struct VSInput
{
    [[vk::location(0)]]
    float3 inPosition;

    [[vk::location(1)]]
    float3 inNormal;

    [[vk::location(2)]]
    float3 inColor;

    [[vk::location(3)]]
    float2 inTexCoord;
};

struct DrawPushConstants
{
    uint drawIndex;
};

struct CameraData
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct FrameData
{
    CameraData camera;
    float time;
    uint frameIndex;
    float2 renderSize;
};

struct ObjectData
{
    float4x4 modelMatrix;
};

struct DrawData
{
    uint objectIndex;
    uint materialIndex;
    uint primitiveIndex;
    uint pad;
};

struct MaterialData
{
    float4 baseColorFactor;

    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    uint flags;

    int baseColorTextureIndex;
    int baseColorSamplerIndex;

    int normalTextureIndex;
    int normalSamplerIndex;

    int metallicRoughnessTextureIndex;
    int metallicRoughnessSamplerIndex;

    int emissiveTextureIndex;
    int emissiveSamplerIndex;
};

[[vk::binding(0, 0)]]
ConstantBuffer<FrameData> frameData;

[[vk::binding(1, 0)]]
StructuredBuffer<ObjectData> objects;

[[vk::binding(2, 0)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(3, 0)]]
StructuredBuffer<PrimitiveData> primitives;

[[vk::binding(4, 0)]]
StructuredBuffer<DrawData> draws;

[[vk::binding(5, 0)]]
Texture2D<float4> textures[];

[[vk::binding(6, 0)]]
SamplerState samplers[];

[[vk::push_constant]]
DrawPushConstants pushConstants;

struct VSOutput
{
    float4 pos : SV_Position;

    [[vk::location(0)]]
    float3 worldPos;

    [[vk::location(1)]]
    float3 normal;

    [[vk::location(2)]]
    float2 uv;

    [[vk::location(3)]]
    float3 color;
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    DrawData draw = draws[pushConstants.drawIndex];
    ObjectData object = objects[draw.objectIndex];

    float4 worldPos = mul(object.modelMatrix, float4(input.inPosition, 1.0));

    VSOutput output;
    output.pos = mul(
        frameData.camera.projectionMatrix,
        mul(frameData.camera.viewMatrix, worldPos)
    );

    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul((float3x3)object.modelMatrix, input.inNormal));
    output.uv = input.inTexCoord;
    output.color = input.inColor;

    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_TARGET
{
    DrawData draw = draws[pushConstants.drawIndex];
    MaterialData material = materials[draw.materialIndex];

    float4 baseColor = material.baseColorFactor * float4(input.color, 1.0);

    if (material.baseColorTextureIndex >= 0 && material.baseColorSamplerIndex >= 0)
    {
        baseColor *= textures[material.baseColorTextureIndex]
            .Sample(samplers[material.baseColorSamplerIndex], input.uv);
    }

    return baseColor;
}