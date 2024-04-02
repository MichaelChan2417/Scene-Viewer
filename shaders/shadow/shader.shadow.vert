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

layout(push_constant) uniform PushConsts 
{
    int lightIdx;
	mat4 view;
    mat4 proj;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void setUpSpotLight(int lightIdx, vec4 tr_pos, vec3 rNormal) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    mat4 curViewMat = lubo.lightViewMatrix[lightIdx];
    mat4 curProjMat = lubo.lightProjMatrix[lightIdx];

    vec3 r_pos = tr_pos.xyz - rNormal * 0.05;
    gl_Position = curProjMat * curViewMat * vec4(r_pos, 1.0);  // old version

    float depth = length(curLightPos - r_pos);
    
    // limit is at index-2
    float d_val = depth / lubo.metadata1[lightIdx][2];

    fragColor = vec3(d_val, d_val, d_val);
}

void setUpSphereLight(int lightIdx, vec4 tr_pos, vec3 rNormal) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    float limit = lubo.metadata1[lightIdx][1];

    vec3 r_pos = tr_pos.xyz - rNormal * 0.02;
    gl_Position = pushConsts.proj * pushConsts.view * vec4(r_pos, 1.0); 

    float dist = length(curLightPos - r_pos);
    float d_val = dist / limit;
    fragColor = vec3(d_val, d_val, d_val);
}

void main() {
    mat4 normalMatrix = transpose(inverse(ubo.instanceModels[gl_InstanceIndex]));
    vec4 tr_pos = ubo.instanceModels[gl_InstanceIndex] * vec4(inPosition, 1.0);
    vec3 rNormal = mat3(normalMatrix) * inNormal;

    // int lightIdx = int(lubo.metadata2[0][0]);
    int lightIdx = pushConsts.lightIdx;

    int lightType = int(lubo.lightPos[lightIdx][3]);

    if (lightType == 0) {
        // spot light
        setUpSpotLight(lightIdx, tr_pos, rNormal);
    }
    else if (lightType == 1) {
        // sphere light
        setUpSphereLight(lightIdx, tr_pos, rNormal);
    }

}