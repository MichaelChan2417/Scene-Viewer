#version 450

const int MAX_INSTANCE = 128;

layout(binding = 1) uniform sampler2D tex2DSampler[MAX_INSTANCE];
layout(binding = 2) uniform samplerCube texCubeSampler[MAX_INSTANCE];

struct InputBlock {
    int inNormalMapIdx;
    int textForm[2];
    int pbrs[3];
    mat4 inNormalMatrix;
    vec3 fragTrack;
};

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

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in InputBlock inputData;


layout(location = 0) out vec4 outColor;

vec3 decodeRGBE(vec4 rgbe)
{
    float av = rgbe.a;

    float exponent = av * 255.0 - 128.0;
    float scale = pow(2.0, exponent);
    vec3 res = rgbe.rgb * scale;
    return res;
}

void main() {

    vec3 rNormal = fragNormal;

    if (inputData.inNormalMapIdx != -1) {
        vec4 mNormal = texture(tex2DSampler[inputData.inNormalMapIdx], fragTexCoord);
        rNormal = mNormal.rgb;
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
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metalness);

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;

    vec3 diffuse = decodeRGBE(texture(texCubeSampler[1], N)) * albedo;
    int roughness_lvl = int(roughness * 10);
    int roughness_idx = roughness_lvl + 2;  // it starts with 2

    vec3 preFilteredColor = decodeRGBE(texture(texCubeSampler[roughness_idx], R));
    vec2 brdf = texture(tex2DSampler[0], vec2(max(dot(N,V), 0.0), roughness)).rg;
    vec3 specular = preFilteredColor * (F * brdf.x + brdf.y);
    
    vec3 ambient = (kD * diffuse + specular);

    vec3 midColor = ambient;
    
    outColor = vec4(midColor, 1.0);
    // outColor = vec4(baseColor * light_decode, 1.0);

    // outColor = vec4(roughness, metalness, 0.0, 1.0);
}