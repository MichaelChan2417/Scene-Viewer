#version 450

const int MAX_INSTANCE = 32;

layout(binding = 0) uniform UniformBufferObject {
    vec3 cameraPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 instanceModels[MAX_INSTANCE];
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 cameraPos;

void main() {
    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);

    fragWorldPos = inPosition;
    cameraPos = ubo.cameraPos;

    fragColor = vec3(gl_Position.z, gl_Position.z, gl_Position.z);
}