#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;
layout(location = 2) in vec4 inFragPositionFromLight;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D uShadowMapSampler;

float getShadowMapDepth()
{
	vec2 UV = (inFragPositionFromLight.xy / inFragPositionFromLight.w) * 0.5 + 0.5;
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

vec3 getShadeColor()
{
	vec3 BaseColor = vec3(1.0, 1.0, 1.0);
	vec3 Light = vec3(3.0, 6.0, 9.0);
	float LightIntensity = 50.0;

	vec3 N = normalize(inFragNormal);
	vec3 L = normalize(Light - inFragPosition);
	float d = distance(inFragPosition, Light);

	float IntensityRatio = min(1.0, 1.0 / max(0.01, (d * d)));

	float Ambient = 0.1;
	float Diffuse = max(0, dot(L, N));

	vec3 Color = LightIntensity * IntensityRatio * BaseColor * (Ambient + Diffuse);
	return Color;
}

void main()
{
	float Vis = getShadowMapVis();
	vec3 ShadeColor = getShadeColor();

	outColor = vec4(Vis * ShadeColor, 1.0);

	//float v = getShadowMapDepth();
	//outColor = vec4(v, v, v, 1.0);
}