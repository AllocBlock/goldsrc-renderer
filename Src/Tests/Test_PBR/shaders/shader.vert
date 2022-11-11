#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in uint inMaterialIndex;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Project;
    mat4 View;
    mat4 Model;
} ubo;

layout(location = 0) out vec3 outFragPosition;
layout(location = 1) out vec3 outFragNormal;
layout(location = 2) out vec3 outFragTangent;
layout(location = 3) out vec2 outTexCoord;
layout(location = 4) flat out uint outFragMaterialIndex;

void main()
{
    gl_Position = ubo.Project * ubo.View * ubo.Model * vec4(inPosition, 1.0);

    outFragPosition = (ubo.Model * vec4(inPosition, 1.0)).xyz;
    mat3 NormalMatrix = transpose(inverse(mat3(ubo.Model)));
    outFragNormal = NormalMatrix * inNormal;
    outFragTangent = NormalMatrix * inTangent;
    outTexCoord = inTexCoord;
    outFragMaterialIndex = inMaterialIndex;
}