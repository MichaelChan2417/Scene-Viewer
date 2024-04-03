#version 450

const int MAX_INSTANCE = 128;
const int MAX_LIGHT = 8;

// referencing https://www.bilibili.com/video/BV1YK4y1T7yY?p=3&vd_source=f9eb1fe5893f11828341e009f807a94d for 
// pcss shadow mapping and also NVIDIA https://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf

layout(binding = 1) uniform sampler2D tex2DSampler[MAX_INSTANCE];
layout(binding = 2) uniform samplerCube texCubeSampler[MAX_INSTANCE];

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


struct InputBlock {
    int inNormalMapIdx;
    int textForm[2];
    mat4 inNormalMatrix;
};

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) flat in InputBlock inputData;


layout(location = 0) out vec4 outColor;


float linear_to_srgb(float x) {
    if (x <= 0.0031308) {
		return x * 12.92;
	} else {
		return 1.055 * pow(x, (1/2.4)) - 0.055;
	}
}


vec3 decodeRGBE(vec4 rgbe)
{
    float av = rgbe.a;

    float exponent = av * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    vec3 res = vec3(0.0);
    res.r = linear_to_srgb(rgbe.r) * scale;
    res.g = linear_to_srgb(rgbe.g) * scale;
    res.b = linear_to_srgb(rgbe.b) * scale;
    return res;
}

vec3 renderSphere(int lightIdx, int sphere_idx) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    float shadowMapSize = lubo.metadata2[lightIdx].w;
    float limit = lubo.metadata1[lightIdx][1];
    float power = lubo.lightDir[lightIdx].w;

    vec3 ptr = fragWorldPos - curLightPos;

    float curDistance = length(ptr);
    float storedDepth = texture(shadowCubeMapSampler[sphere_idx], normalize(ptr)).r * limit;

    if (curDistance > storedDepth) {
        return vec3(0.0, 0.0, 0.0);
    }

    float NdotL = dot(fragNormal, normalize(-ptr));
    vec3 lightColor = lubo.lightColor[lightIdx].xyz;

    float dis_dec_factor = max(0, pow(1 - (curDistance / limit), 4));
    float normal_dec_factor = power / (4 * 3.1415926 * pow(curDistance, 2));

    dis_dec_factor *= normal_dec_factor;
    float fct = dis_dec_factor * NdotL;

    vec3 res = lightColor * vec3(fct, fct, fct);
    return res;
}


vec3 renderSpot(int lightIdx, int spot_idx) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    vec3 curLightDir = lubo.lightDir[lightIdx].xyz;
    mat4 curViewMat = lubo.lightViewMatrix[lightIdx];   
    mat4 curProjMat = lubo.lightProjMatrix[lightIdx];
    float halfFov = lubo.metadata1[lightIdx][1];
    float shadowMapSize = lubo.metadata2[lightIdx].w;

    vec3 ptr = fragWorldPos - curLightPos;
    float viewAngle = acos(dot(normalize(ptr), curLightDir));
    if (viewAngle > halfFov) {
        return vec3(0.0, 0.0, 0.0);
    }

    float curDistance = length(ptr);

    vec4 frag_coord = curProjMat * curViewMat * vec4(fragWorldPos, 1.0);
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

    // this value should be mult with NdotL
    float NdotL = dot(fragNormal, normalize(-ptr));
    pcfVal *= NdotL;

    vec3 res = vec3(pcfVal, pcfVal, pcfVal);

    float blend = lubo.metadata1[lightIdx][3];
    float edge = halfFov*(1-blend);
    if (viewAngle > edge) {
        float px = viewAngle - edge;
        float py = halfFov - viewAngle;
        float portion = py / (px + py);
        vec3 eft = vec3(portion,portion,portion);
        res *= eft;
    }

    float dis_dec_factor = max(0, pow(1 - (curDistance / lubo.metadata1[lightIdx][2]), 4));
    float normal_dec_factor = lubo.lightDir[lightIdx].w / (4 * 3.1415926 * pow(curDistance, 2));
    dis_dec_factor *= normal_dec_factor;
    vec3 fct = vec3(dis_dec_factor, dis_dec_factor, dis_dec_factor);

    res *= fct * lubo.lightColor[lightIdx].xyz;

    return res;
}


void main() {

    vec3 rNormal = fragNormal;

    if (inputData.inNormalMapIdx != -1) {
        vec4 mNormal = texture(tex2DSampler[inputData.inNormalMapIdx], fragTexCoord);
        rNormal.r = linear_to_srgb(mNormal.r);
        rNormal.g = linear_to_srgb(mNormal.g);
        rNormal.b = linear_to_srgb(mNormal.b);
        rNormal -= 0.5f;
        rNormal = rNormal * 2.0f;
        rNormal = mat3(inputData.inNormalMatrix) * rNormal;
    }

    // cube - 0 is always environment; cube - 1 is always env-lambertian
    // vec4 env_light = texture(texCubeSampler[0], rNormal);
    // vec3 light_decode = decodeRGBE(env_light);

    // 2D texture
    vec3 baseColor;
    if (inputData.textForm[0] == 0) {
        vec4 rgbeColor = texture(tex2DSampler[inputData.textForm[1]], fragTexCoord);
        baseColor = rgbeColor.rgb;
    }
    else {
        vec4 rgbeColor = texture(texCubeSampler[inputData.textForm[1]], rNormal);
        baseColor = decodeRGBE(rgbeColor);
    }

    outColor = vec4(0.0, 0.0, 0.0, 1.0);

    // after getting the base color, get light resources
    int totalLightCount = int(lubo.metadata2[0][2]);
    int sphere_idx = 0;
    int spot_idx = 0;

    for (int lightIdx = 0; lightIdx < totalLightCount; lightIdx++) {
        int lightType = int(lubo.lightPos[lightIdx].w);

        // Spot Light
        if (lightType == 0) {
            vec3 pLight = renderSpot(lightIdx, spot_idx);
            outColor += vec4(baseColor * pLight, 0.0);
            spot_idx++;
            // outColor = vec4(pLight, 1.0);
            continue;

        } else if (lightType == 1) {
            vec3 pLight = renderSphere(lightIdx, sphere_idx);
            outColor += vec4(baseColor * pLight, 0.0);
            sphere_idx++;
            continue;
        }

    }

}