#version 450

const int MAX_INSTANCE = 128;

layout(binding = 1) uniform sampler2D tex2DSampler[MAX_INSTANCE];
layout(binding = 2) uniform samplerCube texCubeSampler[MAX_INSTANCE];

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int mType;
layout(location = 3) flat in int mIdx;

layout(location = 0) out vec4 outColor;

vec3 decodeRGBE(vec4 rgbe)
{
    float av = rgbe.a;

    float exponent = av * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    return rgbe.rgb * scale;
}

void main() {

    // cube - 0 is always environment; cube - 1 is always env-lambertian
    vec4 env_light = texture(texCubeSampler[1], fragNormal);
    vec3 light_decode = decodeRGBE(env_light);

    // 2D texture
    vec3 baseColor;
    if (mType == 0) {
        vec4 rgbeColor = texture(tex2DSampler[mIdx], fragTexCoord);
        baseColor = rgbeColor.rgb;
    }
    else {
        vec4 rgbeColor = texture(texCubeSampler[mIdx], fragNormal);
        baseColor = decodeRGBE(rgbeColor);
    }

    outColor = vec4(baseColor * light_decode, 1.0);

    // outColor = vec4(0.0, 0.0, cval, 1.0);
}