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

vec3 decodeRGBE(vec4 rgbe)
{
    float exponent = rgbe.a * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    return rgbe.rgb * scale;
}

void main() {

    vec3 rNormal = fragNormal;

    if (inputData.inNormalMapIdx != -1) {
        vec4 mNormal = texture(tex2DSampler[inputData.inNormalMapIdx], fragTexCoord);
        rNormal = mNormal.rgb;
        rNormal -= 0.5f;
        rNormal = rNormal * 2.0f;
    }

    vec4 rgbeColor = texture(texCubeSampler[0], rNormal);
    vec3 decodedColor = decodeRGBE(rgbeColor);
    outColor = vec4(decodedColor, 1.0);
}