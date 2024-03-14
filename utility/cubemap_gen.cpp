/**
 * We input a cubemap box of environment light and output a cubemap box of irradiance light.
 * and we output a WIDTH*WIDTH*6 cubemap box of radiance light.
 *
 * compile: g++ -std=c++20 -o cube cubemap_gen.cpp -I"D:/STUDY/Libs/stb" -I"D:/STUDY/Libs/glm"
*/

#include <iostream>
#include <string>
#include <random>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <glm/glm.hpp>

constexpr int WIDTH = 32;
constexpr float PI = 3.14159265359f;

void parse_lambertian(unsigned char* image_data, unsigned char* output_data, int width, int height);
void make_ggx(unsigned char* image_data, int width, int height);

glm::vec4 hemi_integrate(unsigned char* image_data, int width, int height, glm::vec3& normal);

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dis(0.0f, 1.0f);

struct Progress {
    static constexpr size_t LENGTH = 40;
    Progress() {
        for (uint32_t i = 0; i < LENGTH; ++i) std::cout << '.';
        for (uint32_t i = 0; i < LENGTH; ++i) std::cout << '\b';
        std::cout.flush();
    }
    void report(size_t step, size_t steps) {
        while (step / float(steps) > p / float(LENGTH)) {
            std::cout << '=';
            std::cout.flush();
            p += 1;
        }
    }
    size_t p = 0;
};


int main(int argc, char** argv) {
    // format is: ./cube in.png --lambertian out.png
    if (argc != 3) {
        std::cerr << "Usage: ./cube <in.png> --[mode]" << std::endl;
        return 1;
    }

    std::string in_file = argv[1];
    std::string out_file;
    std::string mode(argv[2]);
    mode = mode.substr(2);

    // loading image
    int width, height, channels;
    unsigned char* image_data = stbi_load(in_file.c_str(), &width, &height, &channels, 0);

    std::cout << "width: " << width << " height: " << height << " channels: " << channels << std::endl;

    if (channels != 4) {
        std::cerr << "Error: input image must have 4 channels (RGBE)" << std::endl;
        return 1;
    }

    if (!image_data) {
        std::cerr << "Error: could not load image " << in_file << std::endl;
        return 1;
    }

    if (mode == "lambertian") {
        // create a cubemap of size WIDTH*WIDTH*6 to store output
        unsigned char* output_data = new unsigned char[WIDTH * WIDTH * 6 * 4];
        out_file = "out.lambertian.png";
        parse_lambertian(image_data, output_data, width, height);
        stbi_write_png(out_file.c_str(), WIDTH, WIDTH * 6, 4, output_data, WIDTH * 4);
        delete[] output_data;
    }
    // mode should be ggx
    else {
        make_ggx(image_data, width, height);
    }

    // clean resources
    stbi_image_free(image_data);

}

glm::vec3 fetchColor(glm::vec3 dir, unsigned char* image_data, int width, int height) {
    float u, v;
    float x = dir.x;
    float y = dir.y;
    float z = dir.z;
    float absX = fabs(x);
    float absY = fabs(y);
    float absZ = fabs(z);
    int face = 0;
    float uc, vc, u2, v2;

    if (absX > absY && absX > absZ) {
        if (x > 0) {
            face = 0;
            uc = -z / absX;
            vc = -y / absX;
        } else {
            face = 1;
            uc = z / absX;
            vc = -y / absX;
        }
    } else if (absY > absZ) {
        if (y > 0) {
            face = 2;
            uc = x / absY;
            vc = z / absY;
        } else {
            face = 3;
            uc = x / absY;
            vc = -z / absY;
        }
    } else {
        if (z > 0) {
            face = 4;
            uc = x / absZ;
            vc = -y / absZ;
        } else {
            face = 5;
            uc = -x / absZ;
            vc = -y / absZ;
        }
    }

    u = 0.5f * (uc + 1.0f);
    v = 0.5f * (vc + 1.0f);

    int ax = static_cast<int>(u * width);
    int ay = static_cast<int>(v * width);
    ay += face * width;

    // get 4 channels using x and y
    int idx = ay * width * 4 + ax * 4;
    float r = (static_cast<float>(image_data[idx + 0]) + 0.5f) / 255.0f;
    float g = (static_cast<float>(image_data[idx + 1]) + 0.5f) / 255.0f;
    float b = (static_cast<float>(image_data[idx + 2]) + 0.5f) / 255.0f;
    float e = static_cast<float>(image_data[idx + 3]);

    return glm::vec3(r, g, b) * ldexp(1.0f, e - 128);
}

glm::vec3 ImportanceSampleGGX(glm::vec2 v, float roughness, glm::vec3 N) {
    float a = roughness * roughness;

    float phi = 2.0f * PI * v.x;
    float cosTheta = sqrt((1.0f - v.y) / (1.0f + (a * a - 1.0f) * v.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    glm::vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    glm::vec3 up = abs(N.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 tangent = normalize(cross(up, N));
    glm::vec3 bitangent = cross(N, tangent);

    return tangent * H.x + bitangent * H.y + N * H.z;
}

float RadicalInverse_VdC(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

glm::vec2 Hammersley(uint32_t i, uint32_t N) {
    // return glm::vec2(float(i) / float(N),  RadicalInverse_VdC(i));
    // get 2 random float values from 0 - 1
    return glm::vec2(dis(gen), dis(gen));
}

glm::vec3 prefilter(float roughness, glm::vec3 R, unsigned char* image_data, int width, int height) {
    glm::vec3 N = R;
    glm::vec3 V = R;

    const uint32_t SAMPLE_COUNT = 1024u;
    glm::vec3 prefilteredColor(0.0f);

    float totalWeight = 0.0f;
    for (uint32_t i = 0; i < SAMPLE_COUNT; i++) {
        glm::vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        glm::vec3 H = ImportanceSampleGGX(Xi, roughness, N);
        glm::vec3 L = 2.0f * glm::dot(V, H) * H - V;

        float NdotL = glm::max(0.0f, glm::dot(N, L));
        if (NdotL > 0.0f) {
            // use L to find envMapColor
            glm::vec3 envMapColor = fetchColor(L, image_data, width, height);

            prefilteredColor += envMapColor * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= totalWeight;

    return prefilteredColor;
}

void generate_Prefilter_Env_Map(unsigned char* image_data, int width, int height, float roughness) {
    std::string out_file = "out.ggx." + std::to_string(static_cast<int>(roughness * 10)) + ".png";
    unsigned char* output_data = new unsigned char[WIDTH * WIDTH * 6 * 4];

    float halfWidth = 0.5f * WIDTH;
    // we have N = R = V
    for (int face = 0; face < 6; face++) {
        glm::vec3 R;

        for (int y = 0; y < WIDTH; y++) {
            for (int x = 0; x < WIDTH; x++) {
                float h = (static_cast<float>(x) + 0.5f - halfWidth) / halfWidth;
                float v = (static_cast<float>(y) + 0.5f - halfWidth) / halfWidth;
                switch (face) {
                    case 0: // +X
                        R = glm::vec3(1.0f, -v, -h);
                        break;
                    case 1: // -X
                        R = glm::vec3(-1.0f, -v, h);
                        break;
                    case 2: // +Y
                        R = glm::vec3(h, 1.0f, v);
                        break;
                    case 3: // -Y
                        R = glm::vec3(h, -1.0f, -v);
                        break;
                    case 4: // +Z
                        R = glm::vec3(h, -v, 1.0f);
                        break;
                    case 5: // -Z
                        R = glm::vec3(-h, -v, -1.0f);
                        break;
                }

                // we make precomputee for each R
                glm::vec3 res = prefilter(roughness, glm::normalize(R), image_data, width, height);

                int idx = (face * WIDTH + y) * WIDTH * 4 + x * 4;
                float maxv = glm::max(res.r, glm::max(res.g, res.b));
                if (maxv < 1e-32f) {
                    output_data[idx + 0] = 0;
                    output_data[idx + 1] = 0;
                    output_data[idx + 2] = 0;
                    output_data[idx + 3] = 0;
                }
                else {
                    int exp = static_cast<int>(std::ceil(std::log2(maxv)));
                    float scale = std::pow(2.0f, -exp);
                    res *= scale;
                    res = glm::round(res * 255.0f);
                    output_data[idx + 0] = static_cast<unsigned char>(res.r);
                    output_data[idx + 1] = static_cast<unsigned char>(res.g);
                    output_data[idx + 2] = static_cast<unsigned char>(res.b);
                    output_data[idx + 3] = static_cast<unsigned char>(exp + 128);
                }
            }
        }
    }

    stbi_write_png(out_file.c_str(), WIDTH, WIDTH * 6, 4, output_data, WIDTH * 4);
    delete[] output_data;
}

float G_Smith(float Roughness, float NoV, float NoL) {
    float a = Roughness * Roughness;
    float k = a / 2.0f;

    float vis = NoV * (1.0f - k) + k;
    vis /= (NoL * (1.0f - k) + k + 0.00001);

    return vis;

    // float v1 = NoV * 2.0f;
    // float v2 = NoL * 2.0f;

    // float nv2 = NoV * NoV;
    // float nl2 = NoL * NoL;

    // float a = Roughness * Roughness;

    // float d1 = NoV + sqrt(nv2 + (1.0f - nv2) + a);
    // float d2 = NoL + sqrt(nl2 + (1.0f - nl2) + a);

    // return v1 / d1 * v2 / d2;
}

glm::vec3 integrate_BRDF(float roughness, float NoV) {
    glm::vec3 V;
    V.x = sqrt(1.0f - NoV * NoV);
    V.y = 0.0f;
    V.z = NoV;

    float A = 0;
    float B = 0;

    const uint32_t SAMPLE_COUNT = 1024u;
    int cnt = 0;
    for (uint32_t i = 0; i < SAMPLE_COUNT; i++) {
        glm::vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        glm::vec3 H = ImportanceSampleGGX(Xi, roughness, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 L = 2.0f * glm::dot(V, H) * H - V;

        float NdotL = glm::max(0.0f, L.z);
        float NdotH = glm::max(0.0f, H.z);
        float VdotH = glm::max(0.0f, glm::dot(V, H));

        if (NdotL > 0) {
            float G = G_Smith(roughness, NoV, NdotL);
            float G_Vis = G * VdotH / (NdotH * NoV);
            float Fc = pow(1.0f - VdotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
            ++cnt;
        }
    }

    // range should be in 0-1
    float Aval = A / static_cast<float>(SAMPLE_COUNT);
    float Bval = B / static_cast<float>(SAMPLE_COUNT);
    float maxv = glm::max(Aval, Bval);
    if (maxv > 1.0f) {
        Aval /= maxv;
        Bval /= maxv;
    }
    // std::cout << Aval << " " << Bval << std::endl;
    return glm::vec3(Aval, Bval, 0.0f);
}


void generate_LUT(unsigned char* image_data, int width, int height) {
    int out_height = 32;
    int out_width = 32;
    unsigned char* output_data = new unsigned char[out_width * out_height * 3];

    for (int x = 0; x < out_height; x++) {
        for (int y = 0; y < out_width; y++) {
            float roughness = (static_cast<float>(x) + 0.5) / 32.0f;
            float NdotV = (static_cast<float>(y) + 0.5) / 32.0f;

            glm::vec3 res = integrate_BRDF(roughness, NdotV);

            int idx = x * out_width * 3 + y * 3;
            output_data[idx + 0] = static_cast<unsigned char>(res.r * 255.0f);
            output_data[idx + 1] = static_cast<unsigned char>(res.g * 255.0f);
            output_data[idx + 2] = 0;
        }
    }

    stbi_write_png("out.lut.png", out_width, out_height, 3, output_data, out_width * 3);
    delete[] output_data;
}


void make_ggx(unsigned char* image_data, int width, int height) {
    // roughness from 0.0 to 1.0, each increased by 0.1
    for (int i = 0; i < 11; i++) {
        float roughness = static_cast<float>(i) / 10.0f;
        std::cout << "Roughness: " << roughness << std::endl;

        generate_Prefilter_Env_Map(image_data, width, height, roughness);
    }

    generate_LUT(image_data, width, height);
}



void parse_lambertian(unsigned char* image_data, unsigned char* output_data, int width, int height) {

    // each outputdata's pixel is a 1 channel float value, of luminance
    // start by simply fill with 1.0f
    for (int i = 0; i < WIDTH * WIDTH * 6 * 4; i++) {
        output_data[i] = 255;
    }
    float halfWidth = 0.5f * WIDTH;
    Progress progress;

    for (int face = 0; face < 6; face++) {
        // each face is a WIDTH*WIDTH image
        glm::vec3 normal;

        for (int y = 0; y < WIDTH; y++) {
            for (int x = 0; x < WIDTH; x++) {
                float h = (static_cast<float>(x) + 0.5f - halfWidth) / halfWidth;
                float v = (static_cast<float>(y) + 0.5f - halfWidth) / halfWidth;

                switch (face) {
                    case 0: // +X
                        normal = glm::vec3(1.0f, -v, -h);
                        break;
                    case 1: // -X
                        normal = glm::vec3(-1.0f, -v, h);
                        break;
                    case 2: // +Y
                        normal = glm::vec3(h, 1.0f, v);
                        break;
                    case 3: // -Y
                        normal = glm::vec3(h, -1.0f, -v);
                        break;
                    case 4: // +Z
                        normal = glm::vec3(h, -v, 1.0f);
                        break;
                    case 5: // -Z
                        normal = glm::vec3(-h, -v, -1.0f);
                        break;
                }

                glm::vec4 res = hemi_integrate(image_data, width, height, normal);

                int idx = (face * WIDTH + y) * WIDTH * 4 + x * 4;
                output_data[idx + 0] = static_cast<unsigned char>(res.r);
                output_data[idx + 1] = static_cast<unsigned char>(res.g);
                output_data[idx + 2] = static_cast<unsigned char>(res.b);
                output_data[idx + 3] = static_cast<unsigned char>(res.a);

                // print to check
                // std::cout << res.r << " " << res.g << " " << res.b << " " << res.a << std::endl;
            }

            progress.report(y + face * WIDTH, WIDTH * 6);
        }

    }
}



glm::vec4 hemi_integrate(unsigned char* image_data, int width, int height, glm::vec3& normal) {
    glm::vec3 result(0.0f);
    float count = 0.0f;

    normal = glm::normalize(normal);

    float halfWidth = 0.5f * width;
    // image_data's height is 6 * width for cubemap
    for (int face = 0; face < 6; face++) {
        for (int y = 0; y < width; y++) {
            for (int x = 0; x < width; x++) {
                // convert x, y to u, v
                float u = (static_cast<float>(x) + 0.5f - halfWidth) / halfWidth;
                float v = (static_cast<float>(y) + 0.5f - halfWidth) / halfWidth;

                // convert u, v to direction
                glm::vec3 dir;
                switch (face) {
                    case 0: // +X
                        dir = glm::vec3(1.0f, -v, -u);
                        break;
                    case 1: // -X
                        dir = glm::vec3(-1.0f, -v, u);
                        break;
                    case 2: // +Y
                        dir = glm::vec3(u, 1.0f, v);
                        break;
                    case 3: // -Y
                        dir = glm::vec3(u, -1.0f, -v);
                        break;
                    case 4: // +Z
                        dir = glm::vec3(u, -v, 1.0f);
                        break;
                    case 5: // -Z
                        dir = glm::vec3(-u, -v, -1.0f);
                        break;
                }

                // calculate the weight
                dir = normalize(dir);
                float weight = glm::dot(normal, dir);
                if (weight > 0.0f) {
                    // get the pixel value
                    int idx = (face * width + y) * width * 4 + x * 4;
                    float r = (static_cast<float>(image_data[idx + 0]) + 0.5f) / 255.0f;
                    float g = (static_cast<float>(image_data[idx + 1]) + 0.5f) / 255.0f;
                    float b = (static_cast<float>(image_data[idx + 2]) + 0.5f) / 255.0f;
                    int e = static_cast<int>(image_data[idx + 3]);

                    // accumulate the result - with ldexp
                    result += glm::vec3(r, g, b) * weight * ldexp(1.0f, e - 128);
                    count += weight;
                }
            }
        }
    } // end of face

    if (count > 0) {
        result /= static_cast<float>(count);
    }

    float maxv = glm::max(result.r, glm::max(result.g, result.b));
    if (maxv < 1e-32f) {
        return glm::vec4(0.0f);
    }

    int exp = static_cast<int>(std::ceil(std::log2(maxv)));
    float scale = std::pow(2.0f, -exp);
    result *= scale;
    result = glm::round(result * 255.0f);

    return glm::vec4(result, exp + 128);
} // end of hemi_integrate
