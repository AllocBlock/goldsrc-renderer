#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec2 inLightmapCoord;
layout(location = 5) in uint inTexIndex;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 uModel;
    mat4 uView;
    mat4 uProject;
} ubo;

layout(location = 0) out vec3 outFragColor;
layout(location = 1) out vec3 outFragPosition;
layout(location = 2) out vec3 outFragNormal;
layout(location = 3) out vec2 outFragTexCoord;
layout(location = 4) out vec2 outFragLightmapCoord;
layout(location = 5) out uint outTexIndex;

void main()
{
    gl_Position = ubo.uProject * ubo.uView * ubo.uModel * vec4(inPosition, 1.0);

    outFragColor = inColor;
    outFragPosition = (ubo.uModel * vec4(inPosition, 1.0)).xyz;
    outFragNormal = (ubo.uModel * vec4(inNormal, 1.0)).xyz;
    outFragTexCoord = inTexCoord;
    outFragLightmapCoord = inLightmapCoord;
    outTexIndex = inTexIndex;
}