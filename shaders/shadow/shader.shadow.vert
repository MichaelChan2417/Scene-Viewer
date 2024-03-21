#version 450

const int MAX_LIGHT = 8;
const int MAX_INSTANCE = 128;

layout(binding = 0) uniform LightUniformBufferObject {
    vec4 lightPos[MAX_LIGHT];
    vec4 lightDir[MAX_LIGHT];
    vec4 lightColor[MAX_LIGHT];

    mat4 lightViewMatrix[MAX_LIGHT];
    mat4 lightProjMatrix[MAX_LIGHT];

    vec4 metadata1[MAX_LIGHT];
    vec4 metadata2[MAX_LIGHT];
} lubo;

layout(binding = 1) uniform UniformBufferObject {
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

void main() {
    mat4 normalMatrix = transpose(inverse(ubo.instanceModels[gl_InstanceIndex]));
    vec4 tr_pos = ubo.instanceModels[gl_InstanceIndex] * vec4(inPosition, 1.0);

    int lightIdx = int(lubo.metadata2[0][0]);
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    mat4 curViewMat = lubo.lightViewMatrix[lightIdx];
    mat4 curProjMat = lubo.lightProjMatrix[lightIdx];

    gl_Position = curProjMat * curViewMat * tr_pos;

    float depth = length(curLightPos - tr_pos.xyz);
    // limit is at index-2
    float d_val = depth / lubo.metadata1[lightIdx][2];
    fragColor = vec3(d_val, d_val, d_val);
}