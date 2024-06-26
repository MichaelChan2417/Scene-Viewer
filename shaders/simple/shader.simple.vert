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
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 worldPos;

void main() {
    mat4 normalMatrix = transpose(inverse(ubo.instanceModels[gl_InstanceIndex]));
    vec4 tr_pos = ubo.instanceModels[gl_InstanceIndex] * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * tr_pos;

    vec3 rNormal = mat3(normalMatrix) * inNormal;
    vec3 light = mix(vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), dot(rNormal, vec3(0.0, 0.0, 1.0)) * 0.5 + 0.5);
    fragColor = light * inColor;

    worldPos = tr_pos.xyz;


    // float depth = length(vec3(12,0,5) - tr_pos.xyz);
    // limit is at index-2
    // float d_val = depth / 100.0;
    // fragColor = vec3(d_val, d_val, d_val);
    // check
    

}