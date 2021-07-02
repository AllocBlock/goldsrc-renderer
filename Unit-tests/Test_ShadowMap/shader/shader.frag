#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;
layout(location = 2) in vec3 inFragPositionFromLight;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform UniformBufferObject
{
    float ShadowMapWidth;
    float ShadowMapHeight;
} ubo;
layout(binding = 2) uniform sampler2D uShadowMapSampler;

float getShadowMapDepth()
{
	vec2 UV = vec2(inFragPositionFromLight.x / ubo.ShadowMapWidth + 0.5, inFragPositionFromLight.y / ubo.ShadowMapHeight + 0.5);
	return texture(uShadowMapSampler, UV).x;
}

float getShadowMapVis()
{
	float CurrentZ = inFragPositionFromLight.z;
	float ShadowMapZ = getShadowMapDepth();
	if (CurrentZ < ShadowMapZ + 0.1)
		return 1.0;
	else
		return 0.0;
}

void main()
{
	//float Vis = getShadowMapVis();
	//outColor = vec4(Vis, Vis, Vis, 1.0);
	//outColor = vec4(vec3(1.0, 1.0, 1.0) * Vis, 1.0);
	//outColor = vec4(1.0, 1.0, 1.0, 1.0);
	//float Z = getShadowMapDepth();
	//float Z = texture(uShadowMapSampler, inFragPositionFromLight.xy).x;
	//Z = (Z - ubo.ShadowMapCameraNear) / (ubo.ShadowMapCameraFar - ubo.ShadowMapCameraNear);
	//outColor = vec4(Z, Z, Z, 1.0);

	float v = getShadowMapDepth();
	outColor = vec4(v, v, v, 1.0);
}