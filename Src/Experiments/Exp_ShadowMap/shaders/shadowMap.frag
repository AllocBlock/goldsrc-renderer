#version 450

layout(location = 0) in vec4 inFragPosition;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform UniformBufferObject
{
    float ShadowMapCameraNear;
    float ShadowMapCameraFar;
} ubo;

void main()
{
    float NormalizedZ = inFragPosition.z / inFragPosition.w;
	outColor = vec4(NormalizedZ, NormalizedZ, NormalizedZ, 1.0);
}