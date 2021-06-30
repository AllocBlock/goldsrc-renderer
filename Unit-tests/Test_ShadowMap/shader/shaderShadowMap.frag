#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in float outFragZ;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform UniformBufferObject
{
    float ShadowMapCameraNear;
    float ShadowMapCameraFar;
} ubo;

void main()
{
    //float NormalizedZ = (outFragZ - ubo.ShadowMapCameraNear) / (ubo.ShadowMapCameraFar - ubo.ShadowMapCameraNear);
	outColor = vec4(outFragZ, outFragZ, outFragZ, 1.0);
}