#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 BaseColor = vec3(1.0, 1.0, 1.0);
	vec3 Light = vec3(3.0, 6.0, 9.0);

	vec3 L = normalize(Light - inFragPosition);

	float Ambient = 0.1;

	float Diffuse = max(0, dot(L, inFragNormal));

	vec3 Color = BaseColor * (Ambient + Diffuse);
	outColor = vec4(Color, 1.0);
}