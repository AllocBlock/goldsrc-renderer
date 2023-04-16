#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec2 inLightmapCoord;
layout(location = 5) in uint inTexIndex;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Project;
    mat4 View;
    mat4 Model;
} ubo;

layout(location = 0) out vec3 outFragColor;
layout(location = 1) out vec3 outFragPosition;
layout(location = 2) out vec3 outFragNormal;
layout(location = 3) out vec2 outFragTexCoord;
layout(location = 4) out vec2 outFragLightmapCoord;
layout(location = 5) out uint outTexIndex;

void main()
{
    gl_Position = ubo.Project * ubo.View * ubo.Model * vec4(inPosition, 1.0);

    outFragColor = inColor;
    outFragPosition = (ubo.Model * vec4(inPosition, 1.0)).xyz;
    outFragNormal = (ubo.Model * vec4(inNormal, 1.0)).xyz;
    outFragTexCoord = inTexCoord;
    outFragLightmapCoord = inLightmapCoord;
    outTexIndex = inTexIndex;
}