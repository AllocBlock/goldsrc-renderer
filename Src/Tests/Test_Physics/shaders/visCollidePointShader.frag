#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 Color = vec3(1.0, 0.0, 0.0);
	outColor = vec4(Color, 1.0);
}