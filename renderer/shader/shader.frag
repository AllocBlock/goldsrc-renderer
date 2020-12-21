#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec3 inFragPosition;
layout(location = 2) in vec3 inFragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform UniformBufferObject
{
    vec3 uEye;
} ubo;

void main()
{
    vec3 Light = vec3(1.0, 1.0, 1.0);

	vec3 L = normalize(Light - inFragPosition);
	vec3 N = normalize(inFragNormal);
	vec3 V = normalize(ubo.uEye - inFragPosition);
	vec3 H = normalize(L + V);

	float ambient = 0.2;
	float diffuse = max(dot(L, N), 0.0) * 0.5;
	float specular = pow(dot(N, H), 20.0);
	if (dot(L, N) < 0.0) specular = 0.0;

	float fShadow = ambient + diffuse + specular;
    
    outColor = vec4(inFragColor * fShadow, 1.0);
}