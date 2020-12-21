#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec3 inFragPosition;
layout(location = 2) in vec3 inFragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 Light = vec3(1.0, 1.0, 1.0);
    vec3 L = normalize(Light - inFragPosition);
    vec3 N = normalize(inFragNormal);

    float ambient = 0.2;
    float diffuse = max(dot(L, N), 0.0);
    outColor = vec4(inFragColor * (ambient + diffuse), 1.0);
}