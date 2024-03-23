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


void renderSphere(int lightIdx) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    vec3 dir = normalize(worldPos - curLightPos);
    float limit = lubo.metadata1[lightIdx][1];

    float fx = abs(dir.x), fy = abs(dir.y), fz = abs(dir.z);

    float u, v;
    if (fx >= fy && fx >= fz) {
        u = dir.y / fx;
        v = dir.z / fx / 6.0;
        if (dir.x < 0) {
            v -= 5.0/6.0;
        } else {
            v -= 0.5;
        }
    }
    else if (fy >= fx && fy >= fz) {
        u = dir.x / fy;
        v = dir.z / fy / 6.0;
        if (dir.y < 0) {
            v -= 1.0/6.0;
        } else {
            v += 1.0/6.0;
        }
    }
    else {
        u = dir.x / fz;
        v = dir.y / fz / 6.0;
        if (dir.z < 0) {
            v += 0.5;
        } else {
            v += 5.0/6.0;
        }
    }

    u = u * 0.5 + 0.5;
    v = v * 0.5 + 0.5;

    float stored_pre_d = texture(shadowMapSampler[lightIdx], vec2(u, v)).r;
    float storedDepth = stored_pre_d * limit;

    float curDistance = length(worldPos - curLightPos);

    if (curDistance < storedDepth) {
        outColor = vec4(fragColor, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    float dis_dec_factor = pow(1 - (curDistance / limit), 4);
    vec4 fct = vec4(dis_dec_factor, dis_dec_factor, dis_dec_factor, 1.0);
    outColor *= fct;
}


void main() {

    float bias = 0.0;

    // TODO: we need a number of real light count, otherwise => multiple add
    for (int lightIdx = 0; lightIdx < 1; lightIdx++;) {
        int lightType = int(lubo.lightPos[lightIdx].w);

        // for sphere light
        if (lightType == 1) {
            renderSphere(lightIdx);
            continue;
        }

        vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
        vec3 curLightDir = lubo.lightDir[lightIdx].xyz;
        mat4 curViewMat = lubo.lightViewMatrix[lightIdx];   
        mat4 curProjMat = lubo.lightProjMatrix[lightIdx];
        float halfFov = lubo.metadata1[lightIdx][1];

        vec3 ptr = worldPos - curLightPos;
        float viewAngle = acos(dot(normalize(ptr), curLightDir));
        
        // the case when out of fov
        if (viewAngle > halfFov) {
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
            continue;
        }

        float curDistance = length(ptr);

        vec4 frag_coord = curProjMat * curViewMat * vec4(worldPos, 1.0);
        float u = (frag_coord[0]/frag_coord[3] + 1.0) / 2.0;
        float v = (frag_coord[1]/frag_coord[3] + 1.0) / 2.0;

        int x = int(u * 500);
        int y = int(v * 500);

        int range = 1;
        int count = 0;
        float rage = 0.0;

        for (int i = -range; i <= range; i++) {
            for (int j = -range; j <= range; j++) {
                // float stored_pre_d = texture(shadowMapSampler[lightIdx], vec2((x + i + 0.5f) / 500.0, (y + j + 0.5f) / 500.0)).r;
                float stored_pre_d = texture(shadowMapSampler[lightIdx], vec2(u+float(i)/500.0, v+float(j)/500.0)).r;
                float storedDepth = stored_pre_d * lubo.metadata1[lightIdx][2];
                count++;

                if (curDistance + bias < storedDepth) {
                    rage += 1.0;
                } else {
                    rage += 0.05;
                }
            }
        }

        rage /= float(count);

        outColor = vec4(vec3(rage, rage, rage) * fragColor, 1.0);

        float blend = lubo.metadata1[lightIdx][3];
        float edge = halfFov*(1-blend);
        if (viewAngle > edge) {
            float px = viewAngle - edge;
            float py = halfFov - viewAngle;
            float portion = py / (px + py);
            vec4 eft = vec4(portion,portion,portion, 10);
            outColor *= eft;
        }

        float dis_dec_factor = pow(1 - (curDistance / lubo.metadata1[lightIdx][2]), 4);
        vec4 fct = vec4(dis_dec_factor, dis_dec_factor, dis_dec_factor, 1.0);
        outColor *= fct;
    }

}
