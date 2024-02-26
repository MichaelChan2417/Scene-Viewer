#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    // order is +x, -x, +y, -y, +z, -z
    vec3 absNormal = abs(fragNormal);

    float fx = absNormal[0], fy = absNormal[1], fz = absNormal[2];

    // x-direction
    if (fx >= fy && fx >= fz) {
        float vy = fragNormal[1] / fx, vz = fragNormal[2] / fx;

        // +x
        if (fragNormal[0] > 0) {
            float h = 0.5 - vz * 0.5;
            float v = 1.0 / 12.0 - vy / 12.0;
            // float ty = 0.5 - vy * 0.5;
            // float tz = -vz / 12.0 + 1.0 / 12.0;
            outColor = texture(texSampler, vec2(h, v));
        }
        // -x
        else {
            float h = 0.5 + vz * 0.5;
            float v = 0.25 - vy / 12.0;
            // float ty = 0.5 + vy * 0.5;
            // float tz = -vz / 12.0 + 1.0 / 4.0;
            outColor = texture(texSampler, vec2(h, v));
        }
    }

    // y-direction
    else if (fy >= fx && fy >= fz) {
        float vx = fragNormal[0] / fy, vz = fragNormal[2] / fy;

        // +y
        if (fragNormal[1] > 0) {
            float h = vx * 0.5 + 0.5;
            float v = 5.0 / 12.0 + vz / 12.0;
            // float tx = vx * 0.5 + 0.5;
            // float tz = -vz / 12.0 + 5.0 / 12.0;
            outColor = texture(texSampler, vec2(h, v));
        }
        // -y
        else {
            float h = vx * 0.5 + 0.5;
            float v = 7.0 / 12.0 - vz / 12.0;
            // float tx = -vx * 0.5 + 0.5;
            // float tz = -vz / 12.0 + 7.0 / 12.0;
            outColor = texture(texSampler, vec2(h, v));
        }
        
    }

    // z-direction
    else if (fz >= fx && fz >= fy) {
        float vx = fragNormal[0] / fz, vy = fragNormal[1] / fz;

        // +z
        if (fragNormal[2] > 0) {
            float h = 0.5 + vx * 0.5;
            float v = 0.75 - vy / 12.0;
            // float tx = - vx * 0.5 + 0.5;
            // float ty = - vy / 12.0 + 3.0 / 4.0;
            outColor = texture(texSampler, vec2(h, v));
        }
        // -z
        else {
            float h = 0.5 - vx * 0.5;
            float v = 11.0 / 12.0 - vy / 12.0;
            // float tx = vx * 0.5 + 0.5;
            // float ty = - vy / 12.0 + 11.0 / 12.0;
            outColor = texture(texSampler, vec2(h, v));
        }
    }
}