#version 450

const int MAX_LIGHT = 8;

layout(binding = 3) uniform LightUniformBufferObject {
    vec4 lightPos[MAX_LIGHT];
    vec4 lightDir[MAX_LIGHT];
    vec4 lightColor[MAX_LIGHT];

    mat4 lightViewMatrix[MAX_LIGHT];
    mat4 lightProjMatrix[MAX_LIGHT];

    vec4 metadata1[MAX_LIGHT];  // radius - fov/2 - limit - blend
    vec4 metadata2[MAX_LIGHT];
} lubo;

layout(binding = 4) uniform sampler2D shadowMapSampler[MAX_LIGHT];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

void main() {

    float bias = 0.0;

    // TODO: we need a number of real light count, otherwise => multiple add
    for (int light_idx = 0; light_idx < 1; light_idx++) {
        
        outColor = vec4(fragColor, 1.0);

        vec3 curLightPos = lubo.lightPos[light_idx].xyz;
        vec3 curLightDir = lubo.lightDir[light_idx].xyz;
        mat4 curViewMat = lubo.lightViewMatrix[light_idx];   
        mat4 curProjMat = lubo.lightProjMatrix[light_idx];
        float halfFov = lubo.metadata1[light_idx][1];

        vec3 ptr = worldPos - curLightPos;
        float viewAngle = acos(dot(normalize(ptr), curLightDir));
        
        // case when out of fov
        if (viewAngle > halfFov) {
            outColor = vec4(0.0, 1.0, 0.0, 1.0);
            continue;
        }

        float curDistance = length(ptr);

        vec4 frag_coord = curProjMat * curViewMat * vec4(worldPos, 1.0);
        float u = (frag_coord[0]/frag_coord[3] + 1.0) / 2.0;
        float v = (frag_coord[1]/frag_coord[3] + 1.0) / 2.0;

        float stored_pre_d = texture(shadowMapSampler[light_idx], vec2(u, v)).r;
        float storedDepth = stored_pre_d * lubo.metadata1[light_idx][2];
        // float storedDepth = stored_pre_d * 100.0;
        if (curDistance + bias > storedDepth) {
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            float blend = lubo.metadata1[light_idx][3];
            float edge = halfFov*(1-blend);
            if (viewAngle <= edge) {
                outColor = vec4(fragColor, 1.0);
            } else {
                float px = viewAngle - edge;
                float py = halfFov - viewAngle;
                float portion = py / (px + py);
                vec3 eft = vec3(portion,portion,portion);
                outColor = vec4(fragColor * eft, 1.0);
            }

            // distance decrease

        }

        // outColor = vec4(normalize(abs(lubo.lightPos[light_idx].xyz)), 1.0);

        // outColor = vec4(stored_pre_d, stored_pre_d, stored_pre_d, 1.0);
        //outColor = vec4(normalize(frag_coord), 0.0, 1.0);
    }

    // outColor = vec4(fragColor, 1.0);
}