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
layout(binding = 5) uniform samplerCube shadowCubeMapSampler[MAX_LIGHT];

struct InputBlock {
    int inNormalMapIdx;
    int textForm[2];
    int pbrs[3];
    mat4 inNormalMatrix;
    vec3 fragTrack;
};

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) flat in InputBlock inputData;


layout(location = 0) out vec4 outColor;



/*
 * -------------------------------------- Functions --------------------------------------
*/
float linear_to_srgb(float x) {
    if (x <= 0.0031308) {
		return x * 12.92;
	} else {
		return 1.055 * pow(x, (1/2.4)) - 0.055;
	}
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

vec3 decodeRGBE(vec4 rgbe)
{
    float av = rgbe.a;

    float exponent = av * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    vec3 res;
    res.r = linear_to_srgb(rgbe.r) * scale;
    res.g = linear_to_srgb(rgbe.g) * scale;
    res.b = linear_to_srgb(rgbe.b) * scale;
    return res;
}

vec3 getSphereClosestDir(vec3 center, float radius, vec3 startPoint, vec3 dir) {
    vec3 L = center - startPoint;
    float tca = dot(L, dir);
    float d2 = dot(L, L) - tca * tca;
    float radius2 = radius * radius;

    // if shoot inside the sphere, we keep the direction
    if (d2 <= radius2) {
        return dir;
    }

    vec3 projPoint = startPoint + tca * dir;
    vec3 closestAddDir = projPoint - center;
    vec3 closestPoint = center + normalize(closestAddDir) * radius;
    vec3 newDir = normalize(closestPoint - startPoint);
    return newDir;
}

// reference: https://learnopengl.com/PBR/IBL/Specular-IBL
float ggxNormalDistribution(float NdotH, float roughness) {

    if (roughness == 0) {
        roughness = 0.001;
    }

    float a2 = roughness * roughness;
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.1415926 * denom * denom;
    return a2 / denom;
}

float ggxSchlickGTerm(float NdotL, float NdotV, float roughness) {
    float k = roughness * roughness / 2.0;
    float gl = NdotL * (NdotV * (1.0 - k) + k);
    float gv = NdotV * (NdotL * (1.0 - k) + k);
    return gv * gl;
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

    // 2D texture - baseColor
    vec3 albedo;
    vec4 rgbeColor = texture(tex2DSampler[inputData.pbrs[0]], fragTexCoord);
    albedo = rgbeColor.rgb;

    // getting roughness, metalness
    float roughness = linear_to_srgb(texture(tex2DSampler[inputData.pbrs[1]], fragTexCoord).r);
    float metalness = linear_to_srgb(texture(tex2DSampler[inputData.pbrs[2]], fragTexCoord).r);

    // PBR calculating
    vec3 N = normalize(rNormal);
    vec3 V = normalize(inputData.fragTrack);
    vec3 oldR = reflect(-V, N);

    // pre-attribute
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metalness);

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;

    // getting light infos
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
    int totalLightCount = int(lubo.metadata2[0][2]);
    int sphere_idx = 0;
    int spot_idx = 0;

    for (int lightIdx=0; lightIdx<totalLightCount; lightIdx++) {
        int lightType = int(lubo.lightPos[lightIdx].w);
        vec3 curLightPos = lubo.lightPos[lightIdx].xyz;
        float lightRadius = lubo.metadata1[lightIdx].x;

        vec3 R = getSphereClosestDir(curLightPos, lightRadius, fragWorldPos, normalize(oldR));

        if (lightType == 0) {
            // doing spot render
            vec3 curLightDir = lubo.lightDir[lightIdx].xyz;
            mat4 curViewMat = lubo.lightViewMatrix[lightIdx];   
            mat4 curProjMat = lubo.lightProjMatrix[lightIdx];
            float halfFov = lubo.metadata1[lightIdx][1];
            vec3 ptr = fragWorldPos - curLightPos;
            float curDistance = length(ptr);
            float limit = lubo.metadata1[lightIdx][2];
            float power = lubo.lightDir[lightIdx][3];
            float shadowMapSize = lubo.metadata2[lightIdx].w;

            float viewAngle = acos(dot(-R, curLightDir));
            if (viewAngle > halfFov) {
                continue;
            }

            // getting depth value
            vec4 frag_coord = curProjMat * curViewMat * vec4(fragWorldPos, 1.0);
            float u = (frag_coord[0]/frag_coord[3] + 1.0) / 2.0;
            float v = (frag_coord[1]/frag_coord[3] + 1.0) / 2.0;
            int diff = 1;
            int tt_num = 0;
            float pcfVal = 0.0;
            for (int i=-diff; i<=diff; i++) {
                for (int j=-diff; j<=diff; j++) {
                    float stored_pre_d = texture(shadowMapSampler[spot_idx], vec2(u+float(i)/shadowMapSize, v+float(j)/shadowMapSize)).r;
                    float storedDepth = stored_pre_d * limit;

                    if (storedDepth > curDistance) {
                        pcfVal += 1.0;
                    }
                    ++tt_num;
                }
            }
            pcfVal /= float(tt_num);
            pcfVal *= dot(fragNormal, normalize(-ptr));  // diffuse

            float frac = 1.0;
            float blend = lubo.metadata1[lightIdx][3];
            float edge = halfFov*(1-blend);
            if (viewAngle > edge) {
                float px = viewAngle - edge;
                float py = halfFov - viewAngle;
                float portion = py / (px + py);
                frac *= portion;
            }
            float dis_dec_factor = max(0, pow(1 - (curDistance / limit), 4));
            float normal_dec_factor = power / (4 * 3.1415926 * pow(curDistance, 2));
            float frac2 = dis_dec_factor * normal_dec_factor;
            frac *= frac2;

            float lightPart = pcfVal * frac;
            // getting the diffuse part
            vec3 diffuse = vec3(lightPart, lightPart, lightPart) * albedo * lubo.lightColor[lightIdx].rgb;
            // then is GGX
            vec3 H = normalize(V + R);
            float NdotH = max(dot(N, H), 0.0);
            float NdotL = max(dot(N, R), 0.0);
            float NdotV = max(dot(N, V), 0.0);

            float D = ggxNormalDistribution(NdotH, roughness);
            float G = ggxSchlickGTerm(NdotL, NdotV, roughness);

            vec3 ggxTerm = D * G * F * frac2 / (4.0 * NdotV * NdotL + 0.001);
            ggxTerm *= lubo.lightColor[lightIdx].rgb;

            vec3 fnColor = diffuse + ggxTerm;

            // outColor += vec4(fnColor, 0.0);
            outColor += vec4(ggxTerm, 0.0);

            spot_idx++;
            continue;
        }
        else {
            // doing sphere render
            sphere_idx++;
            continue;
        }

    }

    // vec3 diffuse = decodeRGBE(texture(texCubeSampler[1], N)) * albedo;
    // int roughness_lvl = int(roughness * 10);
    // int roughness_idx = roughness_lvl + 2;  // it starts with 2

    // vec3 preFilteredColor = decodeRGBE(texture(texCubeSampler[roughness_idx], R));
    // vec2 brdf = texture(tex2DSampler[0], vec2(max(dot(N,V), 0.0), roughness)).rg;
    // vec3 specular = preFilteredColor * (F * brdf.x + brdf.y);
    
    // vec3 ambient = (kD * diffuse + specular);

    // vec3 midColor = ambient;
    
    // outColor = vec4(midColor, 1.0);
    // outColor = vec4(baseColor * light_decode, 1.0);

    // outColor = vec4(roughness, metalness, 0.0, 1.0);
}