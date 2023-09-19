#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 VP;
    mat4 InversedVP;
    float Strength;
    int SampleNum;
    float SampleRadius;
    float Time;
} ubo;
layout(binding = 1) uniform sampler2D uTexDepth;

layout(location = 0) out vec4 outColor;

#define TOLERANCE 1e-3

float random(vec2 seed) { return fract(sin(dot(seed, vec2(12.9898,78.233))) * 43758.5453123); }
vec3 randomVec3(vec2 seed)
{
    return vec3(random(seed), random(seed + 1), random(seed + 2));
}

float getTolerance(float depth)
{
    return max(TOLERANCE * (1.0 - depth), 1e-7);
}

bool isInScreen(vec2 uv)
{
    return uv.x >= 0.0 && uv.x < 1.0 && uv.y >= 0.0 && uv.y < 1.0;
}

bool isBlocked(vec3 world)
{
    vec4 NDC = ubo.VP * vec4(world, 1);
    vec3 uvw = NDC.xyz / NDC.w;
    vec2 uv = uvw.xy * 0.5 + 0.5;
    if (!isInScreen(uv)) return false;
    float SampleDepth = uvw.z;
    float BlockerDepth = texture(uTexDepth, uv).r;
    return BlockerDepth < SampleDepth - getTolerance(BlockerDepth);
}

void main()
{
    int BlockedNum = 0;
    float CenterDepth = texture(uTexDepth, inFragTexCoord).r;
    vec4 CenterNDC = vec4(inFragTexCoord * 2.0 - 1.0, CenterDepth, 1);
    vec4 Center = ubo.InversedVP * CenterNDC;
    Center /= Center.w;

    for (int i = 0; i < ubo.SampleNum; ++i)
    {
        vec3 Sample3d = Center.xyz + normalize(randomVec3(inFragTexCoord * ubo.Time + i) * 2.0 - 1.0) * ubo.SampleRadius;
        if (isBlocked(Sample3d))
        {
            BlockedNum++;
        }
    }

    float ao = 1.0 - (ubo.Strength * float(BlockedNum - ubo.SampleNum / 2) / float(ubo.SampleNum));
    ao = clamp(ao, 0.0, 1.0);

    outColor = vec4(ao, ao, ao, 1.0);
}