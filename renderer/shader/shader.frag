#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec3 inFragPosition;
layout(location = 2) in vec3 inFragNormal;
layout(location = 3) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform UniformBufferObject
{
    vec3 uEye;
} ubo;

layout(binding = 2) uniform sampler uTexSampler;
layout(binding = 3) uniform texture2D uTexture;

void main()
{
    vec3 Light = vec3(1.0, 1.0, 1.0);

	vec3 L = normalize(Light - inFragPosition);
	vec3 N = normalize(inFragNormal);
	vec3 V = normalize(ubo.uEye - inFragPosition);
	vec3 H = normalize(L + V);

	float ambient = 0.2;
	float diffuse = abs(dot(L, N));
	float specular = pow(abs(dot(N, H)), 20.0);

	float fShadow = ambient + diffuse + specular;

	vec3 TexColor = texture(sampler2D(uTexture, uTexSampler), inFragTexCoord).xyz;
    
    outColor = vec4((inFragColor * 0.5 + TexColor * 0.5) * fShadow, 1.0);
}