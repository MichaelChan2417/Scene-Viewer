/**
 * We input a cubemap box of environment light and output a cubemap box of irradiance light.
 * and we output a WIDTH*WIDTH*6 cubemap box of radiance light.
 *
 * compile: g++ -std=c++20 -o normal image_gen.cpp -I"D:/STUDY/Libs/stb"
*/

#include <iostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main() {
    std::string out_file = "normalMapTest.png";
    unsigned char output_data[1 * 1 * 6 * 4] = { 255, 127, 255, 128,
                                                    255, 127, 255, 128,
                                                    127, 255, 255, 128,
                                                    127, 255, 255, 128,
                                                    127, 127, 255, 128,
                                                    127, 127, 255, 128};
    stbi_write_png(out_file.c_str(), 1, 1 * 6, 4, output_data, 1 * 4);
}