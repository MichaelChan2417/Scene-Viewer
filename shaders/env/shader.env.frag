#version 450

const int MAX_INSTANCE = 128;

layout(binding = 1) uniform sampler2D tex2DSampler[MAX_INSTANCE];
layout(binding = 2) uniform samplerCube texCubeSampler[MAX_INSTANCE];

layout(location = 0) in vec3 fragNormal;
layout(location = 1) flat in inNormalMappingIdx;

layout(location = 0) out vec4 outColor;

vec3 decodeRGBE(vec4 rgbe)
{
    float exponent = rgbe.a * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    return rgbe.rgb * scale;
}

void main() {

    vec3 rNormal = fragNormal;

    if (inNormalMapping != -1) {
        rNormal = texture(tex2DSampler[inNormalMapping], )
    }

    vec4 rgbeColor = texture(texCubeSampler[0], fragNormal);
    vec3 decodedColor = decodeRGBE(rgbeColor);
    outColor = vec4(decodedColor, 1.0);
}