#version 450

const int MAX_INSTANCE = 128;

layout(binding = 1) uniform sampler2D tex2DSampler[MAX_INSTANCE];
layout(binding = 2) uniform samplerCube texCubeSampler[MAX_INSTANCE];

struct InputBlock {
    int inNormalMapIdx;
    int textForm[2];
    mat4 inNormalMatrix;
};

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in InputBlock inputData;


layout(location = 0) out vec4 outColor;


float linear_to_srgb(float x) {
    if (x <= 0.0031308) {
		return x * 12.92;
	} else {
		return 1.055 * pow(x, (1/2.4)) - 0.055;
	}
}


vec3 decodeRGBE(vec4 rgbe)
{
    float av = rgbe.a;

    float exponent = av * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    vec3 res = vec3(0.0);
    res.r = linear_to_srgb(rgbe.r) * scale;
    res.g = linear_to_srgb(rgbe.g) * scale;
    res.b = linear_to_srgb(rgbe.b) * scale;
    return res;
}

void main() {

    vec3 rNormal = fragNormal;

    if (inputData.inNormalMapIdx != -1) {
        vec4 mNormal = texture(tex2DSampler[inputData.inNormalMapIdx], fragTexCoord);
        rNormal.r = linear_to_srgb(mNormal.r);
        rNormal.g = linear_to_srgb(mNormal.g);
        rNormal.b = linear_to_srgb(mNormal.b);
        rNormal -= 0.5f;
        rNormal = rNormal * 2.0f;
        rNormal = mat3(inputData.inNormalMatrix) * rNormal;
    }

    // cube - 0 is always environment; cube - 1 is always env-lambertian
    vec4 env_light = texture(texCubeSampler[1], rNormal);
    vec3 light_decode = decodeRGBE(env_light);

    // 2D texture
    vec3 baseColor;
    if (inputData.textForm[0] == 0) {
        vec4 rgbeColor = texture(tex2DSampler[inputData.textForm[1]], fragTexCoord);
        baseColor = rgbeColor.rgb;
    }
    else {
        vec4 rgbeColor = texture(texCubeSampler[inputData.textForm[1]], rNormal);
        baseColor = decodeRGBE(rgbeColor);
    }

    outColor = vec4(baseColor * light_decode, 1.0);

    // outColor = vec4(light_decode, 1.0);

}