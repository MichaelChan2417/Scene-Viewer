#version 450

const int MAX_INSTANCE = 128;

layout(binding = 0) uniform UniformBufferObject {
    vec3 cameraPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 instanceModels[MAX_INSTANCE];
} ubo;


struct OutputBlock {
    int outNormalMapIdx;
    int textForm[2];
    mat4 outNormalMatrix;
};


// INS:
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 textureMapIdxs;
layout(location = 4) in vec2 inNormalMapIdx;


// OUTS:
layout(location = 0) out vec3 fragNormal;       // this is for lighting
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out OutputBlock outputData;

void main() {
    mat4 normalMatrix = transpose(inverse(ubo.instanceModels[gl_InstanceIndex]));
    vec4 after_Pos = ubo.instanceModels[gl_InstanceIndex] * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * after_Pos;

    vec3 rNormal = mat3(normalMatrix) * inNormal;
    fragTexCoord = texCoord;

    outputData.textForm[0] = int(textureMapIdxs[2]);
    outputData.textForm[1] = int(textureMapIdxs[0]);

    fragNormal = normalize(inPosition);    
    outputData.outNormalMatrix = normalMatrix;
    outputData.outNormalMapIdx = int(inNormalMapIdx[0]);
}