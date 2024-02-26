/**
 * We input a cubemap box of environment light and output a cubemap box of irradiance light.
 * and we output a WIDTH*WIDTH*6 cubemap box of radiance light.
 *
 * compile: g++ -std=c++20 -o cube cubemap_gen.cpp -I"D:/STUDY/Libs/stb" -I"D:/STUDY/Libs/glm"
*/

#include <iostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <glm/glm.hpp>

constexpr int WIDTH = 16;

void parse_lambertian(unsigned char* image_data, unsigned char* output_data, int width, int height);

glm::vec4 hemi_integrate(unsigned char* image_data, int width, int height, glm::vec3& normal);

float luminance(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

int main(int argc, char** argv) {
    // format is: ./cube in.png --lambertian out.png
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <in.png> --[mode] <out.png>" << std::endl;
        return 1;
    }

    std::string in_file = argv[1];
    std::string out_file = argv[3];
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

    // create a cubemap of size WIDTH*WIDTH*6 to store output
    unsigned char* output_data = new unsigned char[WIDTH * WIDTH * 6 * 4];

    if (mode == "lambertian") {
        parse_lambertian(image_data, output_data, width, height);
    }

    // clean resources
    stbi_image_free(image_data);
}

void parse_lambertian(unsigned char* image_data, unsigned char* output_data, int width, int height) {

    // each outputdata's pixel is a 1 channel float value, of luminance
    // start by simply fill with 1.0f
    for (int i = 0; i < WIDTH * WIDTH * 6 * 3; i++) {
        output_data[i] = 255;
    }
    float halfWidth = 0.5f * WIDTH;

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
                std::cout << res.r << " " << res.g << " " << res.b << " " << res.a << std::endl;
            }
        }
    }
}



glm::vec4 hemi_integrate(unsigned char* image_data, int width, int height, glm::vec3& normal) {
    glm::vec3 result(0.0f);
    int count = 0;

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
                    ++count;
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

    int exp = static_cast<int>(std::floor(std::log2(maxv)));
    float scale = std::pow(2.0f, -exp);
    result *= scale;
    result = glm::round(result * 255.0f);

    return glm::vec4(result, exp + 128);
} // end of hemi_integrate
