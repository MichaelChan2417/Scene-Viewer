#version 450

const int MAX_INSTANCE = 128;
const int MAX_LIGHT = 8;

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


struct InputBlock {
    int inNormalMapIdx;
    int textForm[2];
    mat4 inNormalMatrix;
    vec3 worldPos;
};

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in InputBlock inputData;


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


vec3 renderSpot(int lightIdx) {
    vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
    vec3 curLightDir = lubo.lightDir[lightIdx].xyz;
    mat4 curViewMat = lubo.lightViewMatrix[lightIdx];   
    mat4 curProjMat = lubo.lightProjMatrix[lightIdx];
    float halfFov = lubo.metadata1[lightIdx][1];

    vec3 ptr = inputData.worldPos - curLightPos;
    float viewAngle = acos(dot(normalize(ptr), curLightDir));
    if (viewAngle > halfFov) {
        return vec3(0.0, 0.0, 0.0);
    }

    float curDistance = length(ptr);

    vec4 frag_coord = curProjMat * curViewMat * vec4(inputData.worldPos, 1.0);
    float u = (frag_coord[0]/frag_coord[3] + 1.0) / 2.0;
    float v = (frag_coord[1]/frag_coord[3] + 1.0) / 2.0;

    int range = 1;
    int count = 0;
    float pcfVal = 0.0;

    for (int i=-range; i<=range; i++) {
        for (int j=-range; j<=range; j++) {
            float stored_pre_d = texture(shadowMapSampler[lightIdx], vec2(u+float(i)/2000.0, v+float(j)/2000.0)).r;
            float storedDepth = stored_pre_d * lubo.metadata1[lightIdx][2];
            count++;

            if (curDistance < storedDepth) {
                pcfVal += 1.0;
            } else {
                pcfVal += 0.00;
            }
        }
    }
    pcfVal /= float(count);

    // this value should be mult with NdotV
    float NdotV = dot(fragNormal, normalize(-ptr));
    pcfVal *= NdotV;

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

    float dis_dec_factor = pow(1 - (curDistance / lubo.metadata1[lightIdx][2]), 4);
    vec3 fct = vec3(dis_dec_factor, dis_dec_factor, dis_dec_factor);

    res *= fct * lubo.lightColor[lightIdx].xyz;


    float sd = texture(shadowMapSampler[lightIdx], vec2(u, v)).r;
    // res = vec3(sd,sd,sd);

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

    for (int lightIdx = 0; lightIdx < 1; lightIdx++) {
        int lightType = int(lubo.lightPos[lightIdx].w);

        // Spot Light
        if (lightType == 0) {
            vec3 pLight = renderSpot(lightIdx);
            outColor += vec4(baseColor * pLight, 1.0);

            // outColor = vec4(pLight, 1.0);
            continue;

        } else if (lightType == 1) {

        }

    }

}