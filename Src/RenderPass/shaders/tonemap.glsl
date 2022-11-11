#define DEFAULT_GAMMA 2.2
#define GOLDSRC_GAMMA 1.4

float toGamma(float v, float vGamma)
{
	return pow(v, 1/vGamma);
}

vec3 toGamma(vec3 vColor, float vGamma)
{
	return vec3(toGamma(vColor.r, vGamma), toGamma(vColor.g, vGamma), toGamma(vColor.b, vGamma));
}

float fromGamma(float v, float vGamma)
{
	return pow(v, vGamma);
}

vec3 fromGamma(vec3 vColor, float vGamma)
{
	return vec3(fromGamma(vColor.r, vGamma), fromGamma(vColor.g, vGamma), fromGamma(vColor.b, vGamma));
}
