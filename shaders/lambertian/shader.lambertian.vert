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
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 textureMapIdxs;

layout(location = 0) out vec3 fragNormal;       // this is for lighting
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out int mType;
layout(location = 3) out int mIdx;

void main() {
    mat4 normalMatrix = transpose(inverse(ubo.instanceModels[gl_InstanceIndex]));
    vec4 after_Pos = ubo.instanceModels[gl_InstanceIndex] * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * after_Pos;

    vec3 rNormal = mat3(normalMatrix) * inNormal;
    fragNormal = normalize(inPosition);
    fragTexCoord = texCoord;

    if (textureMapIdxs[2] == 0) {
        mType = 0;
    }
    else {
        mType = 1;
    }

    if (textureMapIdxs[0] < 1) {
        mIdx = 0;
    }
    else {
        mIdx = 1;
    }
}