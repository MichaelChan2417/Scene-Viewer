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
layout(binding = 5) uniform samplerCube shadowCubeMapSampler[MAX_LIGHT];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldPos;

layout(location = 0) out vec4 outColor;


void renderSphere(int lightIdx) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    vec3 dir = normalize(worldPos - curLightPos);
    float limit = lubo.metadata1[lightIdx][1];

    float stored_pre_d = texture(shadowCubeMapSampler[lightIdx], dir).r;
    float storedDepth = stored_pre_d * limit;

    float curDistance = length(worldPos - curLightPos);

    if (curDistance < storedDepth) {
        outColor = vec4(fragColor, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    float dis_dec_factor = max(0, pow(1 - (curDistance / limit), 4));
    vec4 fct = vec4(dis_dec_factor, dis_dec_factor, dis_dec_factor, 1.0);
    fct *= vec4(lubo.lightColor[lightIdx].xyz, 1.0);




    outColor *= fct;

    // outColor = vec4(stored_pre_d, stored_pre_d, stored_pre_d, 1.0);

    // outColor = vec4(abs(u), abs(v), 0.0, 1.0);
}


void main() {

    float bias = 0.0;

    // TODO: we need a number of real light count, otherwise => multiple add
    for (int lightIdx = 0; lightIdx < 1; lightIdx++) {
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

        int range = 1;
        int count = 0;
        float avgDepth = 0.0;

        float pcfVal = 0.0;

        for (int i=-range; i <= range; i++) {
            for (int j=-range; j<=range; j++) {
                // float stored_pre_d = texture(shadowMapSampler[lightIdx], vec2((x + i + 0.5f) / 2000.0, (y + j + 0.5f) / 2000.0)).r;
                float stored_pre_d = texture(shadowMapSampler[lightIdx], vec2(u+float(i)/2000.0, v+float(j)/2000.0)).r;
                float storedDepth = stored_pre_d * lubo.metadata1[lightIdx][2];
                count++;

                avgDepth += storedDepth;
                if (curDistance <= storedDepth) {
                    pcfVal += 1.0;
                }
            }
        }

        avgDepth /= float(count);
        pcfVal /= float(count);

        // if (avgDepth > curDistance) {
        //     pcfVal = 1.0;
        // } else {
        //     float w = 2.0 / avgDepth * halfFov * 2.0 * 2000.0;
        //     w = w * (curDistance - avgDepth) / avgDepth;
        //     int rw = int(w) / 2;

        //     rw = max(1, rw);

        //     count = 0;
        //     for (int i=-rw; i<=rw; i++) {
        //         for (int j=-rw; j<=rw; j++) {
        //             float stored_pre_d = texture(shadowMapSampler[lightIdx], vec2(u+float(i)/2000.0, v+float(j)/2000.0)).r;
        //             float storedDepth = stored_pre_d * lubo.metadata1[lightIdx][2];

        //             if (curDistance <= storedDepth) {
        //                 pcfVal += 1.0;
        //             }
        //             count++;
        //         }
        //     }
        //     pcfVal /= float(count);
        // }

        outColor = vec4(vec3(pcfVal, pcfVal, pcfVal) * fragColor, 1.0);

        float blend = lubo.metadata1[lightIdx][3];
        float edge = halfFov*(1-blend);
        if (viewAngle > edge) {
            float px = viewAngle - edge;
            float py = halfFov - viewAngle;
            float portion = py / (px + py);
            vec4 eft = vec4(portion,portion,portion, 1.0);
            outColor *= eft;
        }

        float dis_dec_factor = pow(1 - (curDistance / lubo.metadata1[lightIdx][2]), 4);
        vec4 fct = vec4(dis_dec_factor, dis_dec_factor, dis_dec_factor, 1.0);
        vec4 colorFct = vec4(lubo.lightColor[lightIdx].xyz, 1.0);
        outColor *= fct * colorFct;
    }

    // outColor = vec4(normalize(abs(worldPos)), 1.0);

}
