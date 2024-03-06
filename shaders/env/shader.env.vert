#version 450

const int MAX_INSTANCE = 128;

layout(binding = 0) uniform UniformBufferObject {
    vec3 cameraPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 instanceModels[MAX_INSTANCE];
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inNormalMapping;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out int outNormalMappingIdx;

void main() {
    mat4 normalMatrix = transpose(inverse(ubo.instanceModels[gl_InstanceIndex]));
    gl_Position = ubo.proj * ubo.view * ubo.instanceModels[gl_InstanceIndex] * vec4(inPosition, 1.0);

    vec3 rNormal = mat3(normalMatrix) * inNormal;
    fragNormal = rNormal;

    if (inNormalMapping[0] != -1) {
        outNormalMappingIdx = inNormalMapping[0];
    }
}