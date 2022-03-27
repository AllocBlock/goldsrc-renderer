#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inFragPosition;
layout(location = 1) in vec3 inFragNormal;
layout(location = 2) in vec4 inFragPositionFromLight;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D uShadowMapSampler;

float getShadowMapDepth()
{
	vec2 UV = vec2(inFragPositionFromLight.x / inFragPositionFromLight.w / 2 + 0.5, inFragPositionFromLight.y / inFragPositionFromLight.w / 2 + 0.5);
	return texture(uShadowMapSampler, UV).x;
}

float getShadowMapVis()
{
	float CurrentZ = inFragPositionFromLight.z / inFragPositionFromLight.w;
	float ShadowMapZ = getShadowMapDepth();
	if (CurrentZ < ShadowMapZ + 0.005)
		return 1.0;
	else
		return 0.0;
}

void main()
{
	float Vis = getShadowMapVis();
	outColor = vec4(Vis, Vis, Vis, 1.0);

	//float v = getShadowMapDepth();
	//outColor = vec4(v, v, v, 1.0);
}