#version 450

const int MAX_LIGHT = 8;
const int MAX_INSTANCE = 128;

layout(binding = 0) uniform LightUniformBufferObject {
    vec4 lightPos[MAX_LIGHT];
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

    int lightIdx = int(lubo.metadata[0]);
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    vec3 curLightDir = lubo.lightDir[lightIdx].xyz;
    vec3 curLightUp = lubo.lightUp[lightIdx].xyz;

    // calculating distance
    vec3 ptr = tr_pos.xyz - curLightPos;
    float distance = length(ptr);
    float halfFov = lubo.metadata[1];

    float viewAngle = acos(dot(normalize(ptr), curLightDir));
    if (viewAngle > halfFov) {
        gl_Position = vec4(-1000, -1000, -100.0, 1.0);  // some place never render
    }
    else {
        vec3 curLightRight = cross(curLightDir, curLightUp);
        float targetRadius = tan(viewAngle) / tan(halfFov);
        vec3 planeVec = normalize(ptr) - curLightDir * dot(normalize(ptr), curLightDir);
        planeVec = normalize(planeVec);

        float cosR = dot(planeVec, curLightRight);
        float cosU = -dot(planeVec, curLightUp);

        float depth = distance / lubo.metadata[2];
        // float depth = distance;
        gl_Position = vec4(targetRadius*cosR, targetRadius*cosU, depth, 1.0);
        fragColor = vec3(depth, depth, depth);
    }
}