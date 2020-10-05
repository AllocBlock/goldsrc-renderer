#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in float inTexIndexf;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out float texIndexf;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragShadow;

struct Camera {
	vec3 pos, at, up;
	float fov, aspect, near, far;
};

struct Light {
	vec4 pos, ambient, diffuse, specular;
};

struct Material {
	vec4 ambient, diffuse, specular;
	float shininess;
};

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	Camera cam;
	Light light;
	Material material;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	// 光照计算
	// 哈达玛积：在GLSL中，两个向量*运算时哈达玛积，即(a, b, c)*(x, y ,z) = (ax, by, cz)，而对矩阵则是矩阵乘法
	vec3 L = normalize(-ubo.light.pos.xyz);		//平行光来向
	vec3 N = normalize(inNormal);		//法向量
	vec3 V = normalize(ubo.cam.pos - inPosition);			//观察去向
	vec3 H = normalize(L + V);				//半角矢量

	vec4 ambient = ubo.light.ambient * ubo.material.ambient;

	float d = max(dot(L, N), 0.0);
	vec4 diffuse = d * ubo.light.diffuse * ubo.material.diffuse;

	//2.0 * dot(N, H) - 1.0;
	float s = pow(dot(N, H), ubo.material.shininess);
	vec4 specular = s * ubo.light.specular * ubo.material.specular;;
	if (dot(L, N) < 0.0) specular = vec4(0.0, 0.0, 0.0, 1.0); //照射不到的面

	vec4 fShadow = ambient + diffuse + specular;
	//vec4 fShadow = ambient + diffuse;

    fragColor = inColor;
	texIndexf = inTexIndexf;
	fragShadow = fShadow.xyz;
    fragTexCoord = inTexCoord;
}