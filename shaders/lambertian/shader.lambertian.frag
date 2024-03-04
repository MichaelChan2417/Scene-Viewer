#version 450

const int MAX_INSTANCE = 128;

layout(binding = 1) uniform sampler2D tex2DSampler[MAX_INSTANCE];
layout(binding = 2) uniform samplerCube texCubeSampler[MAX_INSTANCE];

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragTexMapIdxs;
layout(location = 3) flat in int cval;

layout(location = 0) out vec4 outColor;

vec3 decodeRGBE(vec4 rgbe)
{
    float av = rgbe.a;
    if (av == 1) {
        return rgbe.rgb;
    }

    float exponent = av * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    return rgbe.rgb * scale;
}

void main() {

    int spIdx = int(clamp(fragTexMapIdxs[0], 0.0, float(MAX_INSTANCE - 1)));
    // 2D texture
    if (fragTexMapIdxs[3] == 0) {
        vec4 rgbeColor = texture(tex2DSampler[cval], fragTexCoord);
        vec3 decodedColor = decodeRGBE(rgbeColor);
        outColor = vec4(decodedColor, 1.0);
    }
    else {
        vec4 rgbeColor = texture(texCubeSampler[cval], fragNormal);
        vec3 decodedColor = decodeRGBE(rgbeColor);
        outColor = vec4(decodedColor, 1.0);
    }

}