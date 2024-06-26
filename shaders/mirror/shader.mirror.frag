#version 450

const int MAX_INSTANCE = 32;

layout(binding = 1) uniform sampler2D tex2DSampler[MAX_INSTANCE];
layout(binding = 2) uniform samplerCube texCubeSampler[MAX_INSTANCE];

struct InputBlock {
    int inNormalMapIdx;
    int textForm[2];
    mat4 inNormalMatrix;
    vec3 fragTrack;
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
    float exponent = rgbe.a * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    vec3 res;
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

    vec3 ref_dir = reflect(inputData.fragTrack, normalize(rNormal));
    vec4 rgbeColor = texture(texCubeSampler[0], ref_dir);
    vec3 decodedColor = decodeRGBE(rgbeColor);
    outColor = vec4(decodedColor, 1.0);
}