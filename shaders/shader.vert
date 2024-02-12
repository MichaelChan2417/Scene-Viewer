#version 450

const int MAX_INSTANCE = 128;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 instanceModels[MAX_INSTANCE];
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.instanceModels[gl_InstanceIndex] * vec4(inPosition, 1.0);

    vec3 light = mix(vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), dot(inNormal, vec3(0.0, 0.0, 1.0)) * 0.5 + 0.5);
    fragColor = light * inColor;
}