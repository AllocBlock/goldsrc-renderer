struct FSInput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD;
};

[[vk::combinedImageSampler]][[vk::binding(1)]] Texture2D uFontTexture;
[[vk::combinedImageSampler]][[vk::binding(1)]] SamplerState uSampler;

struct SPushConstant
{
    float3 Scale;
    float3 Position;
};

[[vk::push_constant]] SPushConstant uPushConstant;

float4 main(FSInput input) : SV_Target
{
    float sdf = uFontTexture.Sample(uSampler, input.UV).r;
    if (sdf < 0.5)
        discard;
    return float4(1.0, 1.0, 1.0, 1.0);
}