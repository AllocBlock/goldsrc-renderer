#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inFragPosition;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 InverseVP;
    vec4 EyePos;
} ubo;

layout(binding = 1) uniform sampler2D uTexSky;

#define M_1_PI  0.318309886183790671538 // 1/pi
#define M_1_2PI 0.159154943091895335769 // 1/2pi

vec2 worldToLatlongMap(vec3 vDirection)
{
    vec3 p = normalize(vDirection);
    vec2 uv;
    uv.x = atan(-p.y, p.x) * M_1_2PI + 0.5;
    uv.y = acos(p.z) * M_1_PI;
    return uv;
}

void main()
{
    vec4 PosH = vec4(inFragPosition, 0.0, 1.0);
    vec4 PosWorldH = ubo.InverseVP * PosH;
    vec3 W = PosWorldH.xyz / PosWorldH.w;
    vec3 Direction = normalize(W - ubo.EyePos.xyz);

    vec2 uv = worldToLatlongMap(Direction);
    vec4 Color = texture(uTexSky, uv);
    outColor = Color;
}