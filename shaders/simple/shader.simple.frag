#version 450

const int MAX_LIGHT = 8;

layout(binding = 3) uniform LightUniformBufferObject {
    vec4 lightPos[MAX_LIGHT];
    vec4 lightColor[MAX_LIGHT];
    vec4 lightDir[MAX_LIGHT];
    vec4 lightUp[MAX_LIGHT];

    vec4 metadata;  // TODO: this is not enough for all lights
} lubo;

layout(binding = 4) uniform sampler2D shadowMapSampler[MAX_LIGHT];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
    float bias = 0.0;
    // TODO: we need a number of real light count, otherwise => multiple add
    for (int light_idx = 0; light_idx < 1; light_idx++) {
        
        outColor = vec4(fragColor, 1.0);

        vec3 curLightPos = lubo.lightPos[light_idx].xyz;
        vec3 curLightDir = lubo.lightDir[light_idx].xyz;
        vec3 curLightUp = lubo.lightUp[light_idx].xyz;
        float halfFov = lubo.metadata[1];

        vec3 ptr = worldPos - curLightPos;
        float viewAngle = acos(dot(normalize(ptr), curLightDir));
        
        // case when out of fov
        if (viewAngle > halfFov) {
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
            // outColor = vec4(normalize(abs(curLightDir)), 1.0);

            continue;
        }

        float curDistance = length(ptr);

        vec3 curLightRight = cross(curLightDir, curLightUp);
        float targetRadius = tan(viewAngle) / tan(halfFov);
        vec3 planeVec = normalize(ptr) - curLightDir * dot(normalize(ptr), curLightDir);
        planeVec = normalize(planeVec);

        float cosR = dot(planeVec, curLightRight);
        float cosU = -dot(planeVec, curLightUp);

        float u = (targetRadius*cosR + 1.0) / 2.0;
        float v = (targetRadius*cosU + 1.0) / 2.0;

        vec4 shadowMap = texture(shadowMapSampler[light_idx], vec2(u, v));
        float storedDepth = shadowMap.r * lubo.metadata[2];
        // float storedDepth = shadowMap.r;

        if (curDistance + bias > storedDepth) {
            // calculate the percentage
            int x = int(u*500);
            int y = int(v*500);

            //float fn = 0.0;
            //for (int r=x-1; r<=x+1; r++) {
            //    for (int c=y-1; c<=y+1; c++) {
            //        float nu = (float(r)+0.5) / 500.0;
            //        float nv = (float(c)+0.5) / 500.0;
            //        float dd = texture(shadowMapSampler[light_idx], vec2(nu, nv)).r * lubo.metadata[2];
            //        if (curDistance + bias <= dd) {
            //            fn += 1.0/9.0;
            //        }
            //    }
            //}
            //outColor = vec4(fn, fn, fn, 1.0);


            outColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            float blend = lubo.metadata[3];
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
        }

        // outColor = vec4(1.0, 1.0, 1.0, 1.0);
        // outColor = vec4(shadowMap.r,shadowMap.r,shadowMap.r, 1.0);
    }


    // outColor = vec4(fragColor, 1.0);
}