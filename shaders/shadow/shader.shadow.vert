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

void setUpSpotLight(int lightIdx, vec4 tr_pos) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    mat4 curViewMat = lubo.lightViewMatrix[lightIdx];
    mat4 curProjMat = lubo.lightProjMatrix[lightIdx];

    gl_Position = curProjMat * curViewMat * tr_pos;  // old version

    float depth = length(curLightPos - tr_pos.xyz);
    
    // limit is at index-2
    float d_val = depth / lubo.metadata1[lightIdx][2];

    fragColor = vec3(d_val, d_val, d_val);
}

void setUpSphereLight(int lightIdx, vec4 tr_pos) {
    vec3 vertex_pos = tr_pos.xyz;
    vec3 light_pos = lubo.lightPos[lightIdx].xyz;
    vec3 dir = normalize(vertex_pos - light_pos);

    float limit = lubo.metadata1[lightIdx][1];
    float distance = length(vertex_pos - light_pos);
    float d_val = distance / limit;

    float fx = abs(dir.x), fy = abs(dir.y), fz = abs(dir.z);

    if (fx >= fy && fx >= fz) {
        float u = dir.y / fx;
        float v = dir.z / fx / 6.0;
        if (dir.x < 0) {
            gl_Position = vec4(u, v-5.0/6.0, d_val, 1.0);
        } else {
            gl_Position = vec4(u, v-0.5, d_val, 1.0);
        }
    } else if (fy >= fx && fy >= fz) {
        float u = dir.x / fy;
        float v = dir.z / fy / 6.0;
        if (dir.y < 0) {
            gl_Position = vec4(u, v-1.0/6.0, d_val, 1.0);
        } else {
            gl_Position = vec4(u, v+1.0/6.0, d_val, 1.0);
        }
    } else {
        float u = dir.x / fz;
        float v = dir.y / fz / 6.0;
        if (dir.z < 0) {
            gl_Position = vec4(u, v+0.5, d_val, 1.0);
        } else {
            gl_Position = vec4(u, v+5.0/6.0, d_val, 1.0);
        }
    }

    fragColor = vec3(d_val, d_val, d_val);
}

void main() {
    mat4 normalMatrix = transpose(inverse(ubo.instanceModels[gl_InstanceIndex]));
    vec4 tr_pos = ubo.instanceModels[gl_InstanceIndex] * vec4(inPosition, 1.0);
    vec3 rNormal = mat3(normalMatrix) * inNormal;

    int lightIdx = int(lubo.metadata2[0][0]);

    int lightType = int(lubo.lightPos[lightIdx][3]);

    if (lightType == 0) {
        // spot light
        setUpSpotLight(lightIdx, tr_pos);
    }
    else if (lightType == 1) {
        // sphere light
        setUpSphereLight(lightIdx, tr_pos);
    }

}