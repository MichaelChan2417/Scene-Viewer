#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"
#include <iostream>
#include <string>

int writedata(int idx) {
    std::string filenames = "in/modeling_data.0" + std::to_string(idx) + ".tga";
    const char* filename = filenames.c_str();

    int width, height, channels;
    unsigned char* imageData = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha); 

    if (imageData) {
        std::cout << "Successfully loaded image: " << filename << std::endl;
        std::cout << "Image width: " << width << ", height: " << height << ", channels: " << channels << std::endl;

        std::string out_file = "out/model_" + std::to_string(idx-20) + ".png";
        int newWidth = 120;
        int newHeight = 120;
        int newChannels = channels;
        unsigned char* newImageData = new unsigned char[newWidth * newHeight * newChannels];

        int startX = 160; 
        int startY = 300; 
        for (int y = 0; y < newHeight; ++y) {
            for (int x = 0; x < newWidth; ++x) {
                int srcIndex = ((startY + y) * width + (startX + x)) * channels;
                int destIndex = (y * newWidth + x) * newChannels;
                for (int c = 0; c < newChannels; ++c) {
                    newImageData[destIndex + c] = imageData[srcIndex + c];
                }
            }
        }

        stbi_write_jpg(out_file.c_str(), newWidth, newHeight, newChannels, newImageData, 100);


        stbi_image_free(imageData); 
        delete[] newImageData;
    } else {
        std::cerr << "Failed to load image: " << filename << std::endl;
    }

    return 0;
}

void writeNoise(int idx) {

    std::string file_name;
    if (idx < 10) {
        file_name = "noise_in/NubisVoxelCloudNoise.00" + std::to_string(idx) + ".tga";
    } else if (idx < 100) {
        file_name = "noise_in/NubisVoxelCloudNoise.0" + std::to_string(idx) + ".tga";
    }
    else {
        file_name = "noise_in/NubisVoxelCloudNoise." + std::to_string(idx) + ".tga";
    }

    std::string out_file_name = "noise_out/noise_" + std::to_string(idx - 1) + ".png";

    const char* filename = file_name.c_str();

    int width, height, channels;
    unsigned char* imageData = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha); 
    unsigned char* newImageData = new unsigned char[width * height * channels];
    // write to new png
    if (imageData) {
        std::cout << "Successfully loaded image: " << filename << std::endl;
        std::cout << "Image width: " << width << ", height: " << height << ", channels: " << channels << std::endl;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int srcIndex = (y * width + x) * channels;
                int destIndex = (y * width + x) * channels;
                for (int c = 0; c < channels; ++c) {
                    newImageData[destIndex + c] = imageData[srcIndex + c];
                }
            }
        }

        stbi_write_jpg(out_file_name.c_str(), width, height, channels, newImageData, 100);

        stbi_image_free(imageData); 
        delete[] newImageData;
    } else {
        std::cerr << "Failed to load image: " << filename << std::endl;
    }

}

int main() {
    // for (int i = 20; i <= 20+32; i++) {
    //     writedata(i);
    // }

    for (int i = 1; i <= 128; i++) {
        writeNoise(i);
    }
}