struct VSInput
{
    [[vk::location(0)]] float3 inPosition;
    [[vk::location(1)]] float3 inNormal;
    [[vk::location(2)]] float3 inColor;
    [[vk::location(3)]] float2 inTexCoord;
};

struct CameraData
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct FrameDataUBO
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

[[vk::binding(0, 0)]]
ConstantBuffer<FrameDataUBO> frameData;

[[vk::binding(1, 0)]]
StructuredBuffer<ObjectData> objects;

[[vk::binding(2, 0)]]
Sampler2D textureSampler;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceId : SV_InstanceID, uint baseInstance : SV_StartInstanceLocation)
{
    VSOutput output;

    uint objectIndex = instanceId + baseInstance;

    ObjectData object = objects[objectIndex];

    float4 worldPos = mul(object.modelMatrix, float4(input.inPosition, 1.0));
    float4 viewPos  = mul(frameData.camera.viewMatrix, worldPos);
    output.pos      = mul(frameData.camera.projectionMatrix, viewPos);

    // output.pos = float4(input.inPosition.xy * 0.5, 0.0, 1.0);

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;

    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    return textureSampler.Sample(vertIn.fragTexCoord);
    // return float4(1.0, 0.0, 1.0, 1.0); // magenta
}