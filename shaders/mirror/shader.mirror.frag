#version 450

layout(binding = 1) uniform samplerCube texSampler;

layout(location = 0) in vec3 fragTrack;

layout(location = 0) out vec4 outColor;

vec3 decodeRGBE(vec4 rgbe)
{
    float exponent = rgbe.a * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    return rgbe.rgb * scale;
}


void main() {
    vec4 rgbeColor = texture(texSampler, fragTrack);
    vec3 decodedColor = decodeRGBE(rgbeColor);
    outColor = vec4(decodedColor, 1.0);
}