#version 450

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;

layout(location = 0) out vec4 outColor;

void main()
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
	outColor = vec4(Color, 1.0);
}