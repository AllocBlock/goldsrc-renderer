#version 450

#define MAX_TEXTURE_NUM 16
#define M_1_PI  0.318309886183790671538 // 1/pi
#define M_1_2PI 0.159154943091895335769 // 1/2pi

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;
layout(location = 2) in vec3 inFragTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) flat in uint inFragMaterialIndex;

layout(location = 0) out vec4 outColor;

struct SMaterialPBR
{
    vec4 Albedo;
    vec4 OMR;
    int ColorIdx;
    int SpecularIdx;
    int NormalIdx;
    int Padding;
};

layout(binding = 1) uniform SUniformBufferObject
{
    vec4 Eye;
    SMaterialPBR Material;
    uint ForceUseMat;
    uint UseColorTexture;
    uint UseNormalTexture;
    uint UseSpecularTexture;
} ubo;

layout(binding = 2) uniform UniformBufferObject
{
    SMaterialPBR List[16];
} uboMaterial;

layout(binding = 3) uniform sampler uSampler;
layout(binding = 4) uniform texture2D uTextureColorSet[MAX_TEXTURE_NUM];
layout(binding = 5) uniform texture2D uTextureNormalSet[MAX_TEXTURE_NUM];
layout(binding = 6) uniform texture2D uTextureSpecularSet[MAX_TEXTURE_NUM];
layout(binding = 7) uniform sampler2D uTextureSky;
layout(binding = 8) uniform texture2D uTextureSkyIrr;
layout(binding = 9) uniform texture2D uTextureBRDF;

const int POINT_LIGHT_NUMBER = 2;
vec3 gPointLightPositions[POINT_LIGHT_NUMBER] = 
{
    vec3(1.0, -2.0, 1.0),
    vec3(-2.0, -3.0, 3.0),
};

vec3 gPointLightColors[POINT_LIGHT_NUMBER] = 
{
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
};

vec3 gDirectionLightDir = vec3(-1, 1, -1);
vec3 gDirectionLightColor = vec3(5);

SMaterialPBR getCurMaterial()
{
    if (ubo.ForceUseMat > 0u)
        return ubo.Material;
    else
        return uboMaterial.List[inFragMaterialIndex];
}

vec3 gammaCorrect(vec3 vLinear)
{
    return pow(vLinear, vec3(1 / 2.2));
}

vec3 gammaCorrectReverse(vec3 vGamma)
{
    return pow(vGamma, vec3(2.2));
}

const float PI = 3.14159265359;

#define MEDIUMP_FLT_MAX    65504.0
#define MEDIUMP_FLT_MIN    0.00006103515625
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)


float DistributionGGX(vec3 N, vec3 H, float Roughness)
{
    //        float a = Roughness*Roughness;
    //        float a2 = a*a;
    //        float NdotH = max(dot(N, H), 0.0);
    //        float NdotH2 = NdotH*NdotH;
    //
    //        float nom   = a2;
    //        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    //        denom = PI * denom * denom;
    //
    //        return saturateMediump(nom / denom);

    // better ndf with spot light shape
    //vec3 NxH = cross(N, H);
    //float oneMinusNoHSquared = dot(NxH, NxH);
    //float NoH = max(dot(N, H), 0.0);
    //float a = NoH * Roughness;
    //float k = Roughness / (oneMinusNoHSquared + a * a);
    //float d = k * k * (1.0 / PI);
    //return min(d, MEDIUMP_FLT_MAX);

    float a2     = Roughness * Roughness;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float Roughness)
{
    //float r = (Roughness + 1.0);
    //float k = (r*r) / 8.0;
    //
    //float nom   = NdotV;
    //float denom = NdotV * (1.0 - k) + k;
    //return min(nom / denom, MEDIUMP_FLT_MAX);
    float nom   = NdotV;
    float denom = NdotV * (1.0 - Roughness) + Roughness;

    return nom / max(denom, 0.001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float Roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, Roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, Roughness);
    //return min(ggx1 * ggx2, MEDIUMP_FLT_MAX);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec4 sampleSlot(int vIndex, texture2D vTexture, vec4 vDefault)
{
    return vIndex >= 0 ? texture(sampler2D(vTexture, uSampler), inTexCoord) : vDefault;
}

vec3 sampleSlot(int vIndex, texture2D vTexture, vec3 vDefault)
{
    return vIndex >= 0 ? texture(sampler2D(vTexture, uSampler), inTexCoord).rgb : vDefault;
}

float sampleSlot(int vIndex, texture2D vTexture, float vDefault)
{
    return vIndex >= 0 ? texture(sampler2D(vTexture, uSampler), inTexCoord).r : vDefault;
}

vec3 W = inFragPosition;
vec3 N = normalize(inFragNormal);
vec3 T = normalize(inFragTangent); // Tangent
vec3 B = cross(N, T); // Bitangent
vec3 V = normalize(ubo.Eye.xyz - W);
vec3 R = reflect(-V, N);

mat3 TBNMat = mat3(T, B, N);

vec3 pbr(vec3 vColor, float vMetallic, float vRoughness, vec3 vPosToLight, vec3 vLightColor, uint vMatIndex)
{
    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the Albedo color as F0 (Metallic workflow)

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, vColor, vMetallic);
    vec3 L = vPosToLight;
    vec3 H = normalize(V + L);
    float d = length(L);
    float attenuation = 1.0 / (d * d);
    vec3 radiance = vLightColor * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, vRoughness);
    float G   = GeometrySmith(N, V, L, vRoughness);
    vec3  F   = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    vec3 nominator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - vMetallic;

    // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);

    // add to outgoing radiance Lo
    return (kD * vColor / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
}

vec3 Froughness(float NdotV, vec3 F0, float Roughness)
{
    float g = 1.0 - Roughness; // glossiness
    vec3 t = max(vec3(g, g, g), F0);
    return F0 + (t - F0) * pow(1.0 - NdotV, 5.0);
}

vec2 worldToLatlongMap(vec3 vDirection)
{
    vec3 p = normalize(vDirection);
    vec2 uv;
    uv.x = atan(p.z, p.x) * M_1_2PI + 0.5;
    uv.y = acos(p.y) * M_1_PI;
    return uv;
}

vec3 ibl(vec3 BaseColor, float Metallic, float Roughness, float EnvIntensity)
{
    vec2 diffuseUv = worldToLatlongMap(N);
    vec3 DiffuseColor = texture(sampler2D(uTextureSkyIrr, uSampler), diffuseUv).rgb;

    float MaxMipLevel = 7.0;
    float MipLevel = MaxMipLevel * Roughness;
    vec2 specularUv = worldToLatlongMap(R);
    vec3 SpecularColor = textureLod(uTextureSky, specularUv, MipLevel).rgb;

    vec2 brdfUv = vec2(max(dot(N, V), 0.0), 1.0 - Roughness);
    vec2 BRDFScaleBias = texture(sampler2D(uTextureBRDF, uSampler), brdfUv).rg;

    float NdotV = max(dot(N, V), 0.0);
    vec3 F0 = vec3(0.04, 0.04, 0.04);
    F0 = mix(F0, BaseColor, Metallic);
   
    vec3 F = Froughness(NdotV, F0, Roughness);
    vec3 kD = (vec3(1.0, 1.0, 1.0) - F) * (1.0 - Metallic);
    vec3 Diffuse = kD * DiffuseColor * BaseColor;
    vec3 Specular = SpecularColor * (F * BRDFScaleBias.x + BRDFScaleBias.y);
    vec3 IBLAmbientColor = (Diffuse + Specular) * EnvIntensity;
    return IBLAmbientColor;
}

vec3 toAcesFilmic(vec3 _rgb)
{
	// Reference(s):
	// - ACES Filmic Tone Mapping Curve
	//   https://web.archive.org/web/20191027010704/https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
	float aa = 2.51f;
	float bb = 0.03f;
	float cc = 2.43f;
	float dd = 0.59f;
	float ee = 0.14f;
	return clamp( (_rgb*(aa*_rgb + bb) )/(_rgb*(cc*_rgb + dd) + ee), 0.0, 1.0 );
}

vec3 shade()
{
    SMaterialPBR Mat = getCurMaterial();

    vec3 BaseColor = Mat.Albedo.rgb;
    if (ubo.UseColorTexture > 0)
        BaseColor = sampleSlot(Mat.ColorIdx, uTextureColorSet[uint(Mat.ColorIdx)], BaseColor);

    if (Mat.NormalIdx >= 0 && ubo.UseNormalTexture > 0)
    {
        vec3 Normal = sampleSlot(Mat.NormalIdx, uTextureNormalSet[uint(Mat.NormalIdx)], N).xyz;
        Normal = Normal * 2.0 - 1.0;
        N = TBNMat * Normal;
    }
    
    vec4 OMR = Mat.OMR;
    if (ubo.UseSpecularTexture > 0)
        OMR = sampleSlot(Mat.SpecularIdx, uTextureSpecularSet[uint(Mat.SpecularIdx)], OMR);

    //float AO = OMR.r;
    float AO = 1.0;
    float Metallic = OMR.g;
    float Roughness = OMR.b;

    //return BaseColor;
    //return W;
    //return N;
    //return T;
    //return Normal;
    //return OMR.rgb;

    // use texture color
    //vec3 TexCoord = normalize(inFragNormal);
    //Mat.Albedo = texture(uSkyCubeSampler, TexCoord);

    vec3 LumColor = vec3(0.0);
    // point light
    for(int i = 0; i < POINT_LIGHT_NUMBER; ++i)
    {
        vec3 L = normalize(gPointLightPositions[i] - W);
        LumColor += pbr(BaseColor, Metallic, Roughness, L, gPointLightColors[i], inFragMaterialIndex) * AO;
    }

    // directional light
    {
        vec3 L = normalize(-gDirectionLightDir);
        LumColor += pbr(BaseColor, Metallic, Roughness, L, gDirectionLightColor, inFragMaterialIndex) * AO;
    }

    // IBL
    LumColor += ibl(BaseColor, Metallic, Roughness, 1.0) * AO;

    vec3 Result = LumColor;

    // HDR tonemapping
    //Result = Result / (Result + vec3(1.0));
    Result = toAcesFilmic(Result);

    return Result;
}

void main()
{
    outColor = vec4(shade(), 1.0);
}