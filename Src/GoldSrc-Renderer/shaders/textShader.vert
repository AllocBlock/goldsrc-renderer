struct VSInput
{
[[vk::location(0)]] float2 Position : POSITION;
[[vk::location(1)]] float2 UV : TEXCOORD;
};

struct UBO
{
    float4x4 Proj;
    float4x4 View;
    float3 EyePosition;
    float3 EyeDirection;
};

[[vk::binding(0)]] cbuffer ubo { UBO ubo; }

struct SPushConstant
{
    float3 Scale;
    float3 Position;
};

[[vk::push_constant]] SPushConstant uPushConstant;

struct VSOutput
{
	float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD;
};

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
    
    float3 Z = normalize(-ubo.EyeDirection);
    float3 X = normalize(cross(float3(0.0, 1.0, 0.0), Z));
    float3 Y = normalize(cross(Z, X));
    float3x3 Rotate = float3x3(X, Y, Z);
    float3 World = mul(Rotate, float3(input.Position.xy, 0.0) * uPushConstant.Scale) + uPushConstant.Position;
    
	VSOutput output = (VSOutput)0;
    output.Pos = mul(ubo.Proj, mul(ubo.View, float4(World, 1.0)));
    output.UV = input.UV;
	return output;
}