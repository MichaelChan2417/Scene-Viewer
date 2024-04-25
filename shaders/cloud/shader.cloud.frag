#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 cameraPos;

layout(binding = 1) uniform sampler2D cloudSampler[32];
layout(binding = 2) uniform sampler3D cloudNoiseSampler;
layout(location = 0) out vec4 outColor;
layout(depth_any) out float gl_FragDepth;

layout(binding = 3) uniform LightUniformBufferObject {
    vec4 lightPos;
    vec4 lightColor;
    vec4 metadata;
} cubo;


const float XMIN = -5.0;
const float XMAX = 5.0;
const float YMIN = -5.0;
const float YMAX = 5.0;
const float ZMIN = 5.0;
const float ZMAX = 8.0;
const float EPSILON = 0.12;
const float ROU = 1.8;
const float DENSITY_STEP_SIZE = 0.12;
// REFERRENCE: UNITY URP https://github.com/NikLever/Unity-URP-Cookbook/blob/16436d75b3ba180f62a03065fdcc134c41084b28/Assets/Scripts/HLSL/Raymarch.hlsl#L12
const float DARKNESSLIMIT = 0.15;
const float CLOUD_MOVE_SPEED = 0.12;


float linear_to_srgb(float x) {
    if (x <= 0.0031308) {
		return x * 12.92;
	} else {
		return 1.055 * pow(x, (1/2.4)) - 0.055;
	}
}
// --------------------------------------------
// referrencing: nubis-3 idea, but I use structures to hold specific values
struct VoxelCloudModelData {
    float dimensionalProfile;
    float detailType;
    float densityScale;
};

struct VoxelCloudDensitySamples {
    float mProfile; 
    float mFull;    
};


// Referrence from code by nubis-3
// Function to erode a value given an erosion amount. A simplified version of SetRange.
//
float ValueErosion(float inValue, float inOldMin) {
	// derrived from Set-Range, this function uses the oldMin to erode or inflate the input value. 
    // - inValues inflate while + inValues erode
	float old_min_max_range = (1.0 - inOldMin);
    float clamped_normalized = clamp((inValue - inOldMin) / old_min_max_range, 0.0, 1.0);
	return (clamped_normalized);
}

// Sample the general big cloud model
VoxelCloudModelData sampleCloud(vec3 pos) {
    VoxelCloudModelData data;
    data.dimensionalProfile = 0.0;
    data.detailType = 0.0;
    data.densityScale = 0.0;

    if (pos.x < XMIN || pos.x > XMAX || pos.y < YMIN || pos.y > YMAX || pos.z < ZMIN || pos.z > ZMAX) {
        return data;
    }

    float layerF = (pos.z - ZMIN) / (ZMAX - ZMIN) * 32.0;
    int layer = int((pos.z - ZMIN) / (ZMAX - ZMIN) * 32.0);
    int layer2 = min(layer+1, 31);
    vec2 uv = vec2((pos.x - XMIN) / (XMAX - XMIN), (pos.y - YMIN) / (YMAX - YMIN));

    vec4 cloudSample = texture(cloudSampler[layer], uv);
    cloudSample.r = linear_to_srgb(cloudSample.r);
    cloudSample.g = linear_to_srgb(cloudSample.g);
    cloudSample.b = linear_to_srgb(cloudSample.b);
    cloudSample.a = linear_to_srgb(cloudSample.a);
    
    if (layer == layer2) {
        data.dimensionalProfile = cloudSample.r;
        data.detailType = cloudSample.g;
        data.densityScale = cloudSample.b;

        return data;
    }

    vec4 cloudSample2 = texture(cloudSampler[layer2], uv);
    cloudSample2.r = linear_to_srgb(cloudSample2.r);
    cloudSample2.g = linear_to_srgb(cloudSample2.g);
    cloudSample2.b = linear_to_srgb(cloudSample2.b);
    cloudSample2.a = linear_to_srgb(cloudSample2.a);

    float d1 = layerF - float(layer);
    float d2 = 1.0 - d1;

    data.dimensionalProfile = cloudSample.r * d2 + cloudSample2.r * d1;
    data.detailType = cloudSample.g * d2 + cloudSample2.g * d1;
    data.densityScale = cloudSample.b * d2 + cloudSample2.b * d1;
    return data;
}

vec2 rayDist(vec3 dir) {
    vec3 t0 = (vec3(XMIN, YMIN, ZMIN) - cameraPos) / dir;
    vec3 t1 = (vec3(XMAX, YMAX, ZMAX) - cameraPos) / dir;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    float dstA = max(max(tmin.x, tmin.y), tmin.z);
    float dstB = min(min(tmax.x, tmax.y), tmax.z);

    float dstToBox = max(dstA, 0.0);
    float dstInsideBox = max(dstB-dstToBox, 0.0);
    return vec2(dstToBox, dstInsideBox);
}

// Getting the Detailed Noise Cloud Pack
float getUprezzedVoxelCloudDensity(vec3 pos, VoxelCloudModelData modelData, float distance_fraction) {
    vec3 cpos = pos - vec3(cubo.metadata.x, cubo.metadata.y, 0) * CLOUD_MOVE_SPEED;
    vec3 uvd = vec3((cpos.x - XMIN) / (XMAX - XMIN), (cpos.y - YMIN) / (YMAX - YMIN), (cpos.z - ZMIN) / (ZMAX - ZMIN));
    vec4 noiseSample = texture(cloudNoiseSampler, uvd);
    noiseSample.r = linear_to_srgb(noiseSample.r);
    noiseSample.g = linear_to_srgb(noiseSample.g);
    noiseSample.b = linear_to_srgb(noiseSample.b);
    noiseSample.a = linear_to_srgb(noiseSample.a);

    float wispy_noise = mix(noiseSample.r, noiseSample.g, modelData.dimensionalProfile);

    float billowy_type_gradient = pow(modelData.dimensionalProfile, 0.25);
    float billowy_noise = mix(noiseSample.b * 0.3, noiseSample.a * 0.3, billowy_type_gradient);

    float noise_composite = mix(wispy_noise, billowy_noise, modelData.detailType);

    // HIGH FREQUENCY
    float v1 = 1.0 - pow(abs(abs(noiseSample.g * 2.0 - 1.0) * 2.0 - 1.0), 4.0);
    float v2 = pow(abs(abs(noiseSample.a * 2.0 - 1.0) * 2.0 - 1.0), 2.0);
    float hhf = clamp(v1, v2, modelData.detailType);
    float range_blender = 0.95; // TODO: to be added
    noise_composite = mix(hhf, noise_composite, range_blender);

	// Composote Noises and use as a Value Erosion
	float uprezzed_density = ValueErosion(modelData.dimensionalProfile, noise_composite);
    float powered_density_scale = pow(clamp(modelData.densityScale, 0.0, 1.0), 4.0);
    uprezzed_density *= powered_density_scale;

    // this is a trick
    uprezzed_density = pow(uprezzed_density, mix(0.3, 0.6, max(EPSILON, powered_density_scale)));

    // HIGH FREQUENCY
    uprezzed_density = pow(uprezzed_density, mix(0.5, 1.0, distance_fraction)) * mix(0.666, 1.0, distance_fraction);
	

    return uprezzed_density;
}

VoxelCloudDensitySamples getVoxelDensity(VoxelCloudModelData voxelModel, vec3 curPos, float distance_fraction) {
    VoxelCloudDensitySamples densitySamples;
    if (voxelModel.dimensionalProfile > 0.0) {
        densitySamples.mProfile = voxelModel.dimensionalProfile * voxelModel.densityScale;
        
        densitySamples.mFull = getUprezzedVoxelCloudDensity(curPos, voxelModel, distance_fraction);
    }
    else {
        densitySamples.mFull = densitySamples.mProfile = 0.0;
    }

    return densitySamples;
}

float getDirectLightRadiance(vec3 pos) {
    vec3 lightPos = cubo.lightPos.xyz;
    vec3 lightDir = normalize(lightPos - pos);

    vec3 newPos = pos;
    float lightAccumulation = 0.0;
    float transmittance = 0.0;

    float b_scale = 1.0;

    for (int i=0; i<20; i++) {
        VoxelCloudModelData nVoxelModel = sampleCloud(newPos);
        VoxelCloudDensitySamples densitySamples = getVoxelDensity(nVoxelModel, newPos, 1.0);

        lightAccumulation += densitySamples.mFull;
        b_scale *= (1 - densitySamples.mFull);

        newPos += lightDir * 0.15;

        // if newPos is outside the cloud, break
        if (newPos.x < XMIN || newPos.x > XMAX || newPos.y < YMIN || newPos.y > YMAX || newPos.z < ZMIN || newPos.z > ZMAX) {
            break;
        }
    }

    float lightTransimssion = exp(-lightAccumulation);
    float shadow = DARKNESSLIMIT + (1.0 - DARKNESSLIMIT) * lightTransimssion;
    // float shadow = DARKNESSLIMIT + (1.0 - DARKNESSLIMIT) * b_scale;

    return shadow;
}


void main() {
    vec3 dir = normalize(fragWorldPos - cameraPos);

    vec2 dstInfo = rayDist(dir);

    int steps = 160;
    vec3 startPos = cameraPos + dir * dstInfo.x;

    float outRadiance = 0.0;
    float curTransmittance = 1.0;
    vec3 curPos = startPos;
    float alpha = 0.0;

    for (int i=0; i<steps; i++) {

        // sample cloud to get big model
        VoxelCloudModelData voxelModel = sampleCloud(curPos);

        float distance_fraction = min(1.0, length(curPos - startPos) / dstInfo.y);
        VoxelCloudDensitySamples densitySamples = getVoxelDensity(voxelModel, curPos, distance_fraction);
        
        if (densitySamples.mFull <= 0.04) {
            curPos += dir * DENSITY_STEP_SIZE;
            continue;
        }
        float scatter_radiance = getDirectLightRadiance(curPos);

        outRadiance += densitySamples.mFull * scatter_radiance * curTransmittance;
        curTransmittance *= exp(-ROU * densitySamples.mFull);
        // curTransmittance *= (1 - densitySamples.mFull);
        // outRadiance += densitySamples.mFull;

        alpha += densitySamples.mFull / 12.0;

        curPos += dir * DENSITY_STEP_SIZE * 0.06;
    }

    outColor = vec4(cubo.lightColor.xyz * outRadiance, alpha);
    
}

