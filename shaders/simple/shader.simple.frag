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


vec3 renderSphere(int lightIdx, int sphere_idx) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    vec3 dir = normalize(worldPos - curLightPos);
    float limit = lubo.metadata1[lightIdx][1];
    float power = lubo.lightDir[lightIdx].w;

    float stored_pre_d = texture(shadowCubeMapSampler[sphere_idx], dir).r;
    float storedDepth = stored_pre_d * limit;

    float curDistance = length(worldPos - curLightPos);

    if (curDistance < storedDepth) {
        return fragColor;
    } else {
        return vec3(0.0, 0.0, 0.0);
    }

    float dis_dec_factor = max(0, pow(1 - (curDistance / limit), 4));
    dis_dec_factor *= power / (4 * 3.1415926 * pow(curDistance, 2));
    vec3 fct = vec3(dis_dec_factor, dis_dec_factor, dis_dec_factor);
    fct *= vec3(lubo.lightColor[lightIdx].xyz);

    return fragColor * fct;
}


vec3 renderSpot(int lightIdx, int spot_idx) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    vec3 curLightDir = lubo.lightDir[lightIdx].xyz;
    mat4 curViewMat = lubo.lightViewMatrix[lightIdx];   
    mat4 curProjMat = lubo.lightProjMatrix[lightIdx];
    float halfFov = lubo.metadata1[lightIdx][1];
    float shadowMapSize = lubo.metadata2[lightIdx].w;

    vec3 ptr = worldPos - curLightPos;
    float viewAngle = acos(dot(normalize(ptr), curLightDir));
    
    // the case when out of fov
    if (viewAngle > halfFov) {
        return vec3(0.0, 0.0, 0.0);
    }

    float curDistance = length(ptr);

    vec4 frag_coord = curProjMat * curViewMat * vec4(worldPos, 1.0);
    float u = (frag_coord[0]/frag_coord[3] + 1.0) / 2.0;
    float v = (frag_coord[1]/frag_coord[3] + 1.0) / 2.0;

    int diff = 1;
    int tt_num = 0;
    float avgDepth = 0.0;

    float pcfVal = 0.0;

    for (int i=-diff; i<=diff; i++) {
        for (int j=-diff; j<=diff; j++) {
            float stored_pre_d = texture(shadowMapSampler[spot_idx], vec2(u+float(i)/shadowMapSize, v+float(j)/shadowMapSize)).r;
            float storedDepth = stored_pre_d * lubo.metadata1[lightIdx][2];

            if (storedDepth < curDistance) {
                avgDepth += storedDepth;
                ++tt_num;
            }
        }
    }

    avgDepth /= float(tt_num);

    if (tt_num == 0) {
        pcfVal = 1.0;
    }
    else {
        float lightRadius = lubo.metadata1[lightIdx][0];
        float frac1 = (curDistance - avgDepth) / avgDepth;
        float w = shadowMapSize * lightRadius * frac1 / (curDistance * tan(halfFov));
        int rw = int(w) / 4;

        rw = min(10, rw);

        tt_num = 0;
        for (int i=-rw; i<=rw; i++) {
            for (int j=-rw; j<=rw; j++) {
                float stored_pre_d = texture(shadowMapSampler[spot_idx], vec2(u+float(i)/shadowMapSize, v+float(j)/shadowMapSize)).r;
                float storedDepth = stored_pre_d * lubo.metadata1[lightIdx][2];

                if (curDistance <= storedDepth) {
                    pcfVal += 1.0;
                }
                tt_num++;
            }
        }
        pcfVal /= float(tt_num);
    }

    vec3 res = vec3(pcfVal, pcfVal, pcfVal) * fragColor;

    float blend = lubo.metadata1[lightIdx][3];
    float edge = halfFov*(1-blend);
    if (viewAngle > edge) {
        float px = viewAngle - edge;
        float py = halfFov - viewAngle;
        float portion = py / (px + py);
        vec3 eft = vec3(portion,portion,portion);
        res *= eft;
    }

    float dis_dec_factor = pow(1 - (curDistance / lubo.metadata1[lightIdx][2]), 4);
    vec3 fct = vec3(dis_dec_factor, dis_dec_factor, dis_dec_factor);
    vec3 colorFct = vec3(lubo.lightColor[lightIdx].xyz);
    res *= fct * colorFct;
    
    return res;
}


void main() {

    float bias = 0.0;
    int totalLightCount = int(lubo.metadata2[0][2]);
    int sphere_idx = 0;
    int spot_idx = 0;

    outColor = vec4(0.0, 0.0, 0.0, 1.0);

    for (int lightIdx = 0; lightIdx < totalLightCount; lightIdx++) {
        int lightType = int(lubo.lightPos[lightIdx].w);

        // for sphere light
        if (lightType == 1) {
            vec3 res = renderSphere(lightIdx, sphere_idx);
            outColor += vec4(res, 1.0);
            ++sphere_idx;
        }
        else if (lightType == 0) {
            renderSpot(lightIdx, spot_idx);
        }

        
    }

}
