#include "../scene_viewer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void generateSinglePixel(std::vector<double>& val, std::string& file_name);

void SceneViewer::createTextureImagesWithViews() {
    // start with 2D's
    std::unordered_map<std::string, int> texture2D2Idx = scene_config.texture2D2Idx;
    for (auto& [file_name, idx] : texture2D2Idx) {
        std::cout << "loading 2D texture " << file_name << " with idx " << idx << std::endl;
        createTextureImage2D(file_name, idx);
        texture2DImageViews[idx] = createImageView2D(texture2DImages[idx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // then cube's
    std::unordered_map<std::string, int> textureCube2Idx = scene_config.textureCube2Idx;
    for (auto& [file_name, idx] : textureCube2Idx) {
        std::cout << "loading cube texture " << file_name << " with idx " << idx << std::endl;
        createTextureImageCube(file_name, idx);
        textureCubeImageViews[idx] = createImageViewCube(textureCubeImages[idx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void SceneViewer::createTextureImage2D(const std::string& file_name, int idx) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(file_name.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    std::cout << "texture image size: " << texWidth << "x" << texHeight << std::endl;

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, 0, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture2DImages[idx], texture2DImageMemorys[idx], 1);

    transitionImageLayout(texture2DImages[idx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        copyBufferToImage(stagingBuffer, texture2DImages[idx], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
    transitionImageLayout(texture2DImages[idx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void SceneViewer::createTextureImageCube(const std::string& file_name, int idx) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(file_name.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    std::cout << "texture image size: " << texWidth << "x" << texHeight << std::endl;

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texWidth, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureCubeImages[idx], textureCubeImageMemorys[idx], 6);

    transitionImageLayout(textureCubeImages[idx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);
        copyBufferToImage(stagingBuffer, textureCubeImages[idx], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texWidth), 6);
    transitionImageLayout(textureCubeImages[idx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}


void SceneViewer::createTextureImageView() {
    // textureImageView = createImageView2D(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    // textureImageView = createImageViewCube(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void SceneViewer::createTextureSampler() {

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler2D) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    // then for cube // TODO: seems we don't need this?? 
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSamplerCube) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}


void SceneViewer::texturePrepare() {
    // first is environment
    std::shared_ptr<sconfig::Environment> env = scene_config.environment;
    scene_config.textureCube2Idx[env->texture_src] = scene_config.textureCube2Idx.size();

    // TODO: then is a environment light sampler
    std::cout << "TODO: in texture" << std::endl;

    // then for all materials => load texture
    for (auto& [id, mat] : scene_config.id2material) {
        std::cout << id << " " << mat->matetial_type << std::endl;

        if (mat->matetial_type == MaterialType::lambertian) {
            std::shared_ptr<sconfig::Lambertian> detail = std::get<std::shared_ptr<sconfig::Lambertian>>(mat->matetial_detail);
            if (std::holds_alternative<std::vector<double>>(detail->albedo)) {
                std::string temp_file_name = "temp\\lambertian_" + std::to_string(id) + "_albedo.png";
                generateSinglePixel(std::get<std::vector<double>>(detail->albedo), temp_file_name);
                detail->albedo = temp_file_name;
                detail->albedo_type = TextureType::texture2D;
            }
            // now it's definitely a string (file)
            std::string filename = std::get<std::string>(detail->albedo);
            if (scene_config.textureCube2Idx.find(filename) != scene_config.textureCube2Idx.end() || scene_config.texture2D2Idx.find(filename) != scene_config.texture2D2Idx.end() ){
                continue;
            }
            if (detail->albedo_type == TextureType::texture2D) {
                scene_config.texture2D2Idx[filename] = scene_config.texture2D2Idx.size();
            }
            else {
                scene_config.textureCube2Idx[filename] = scene_config.textureCube2Idx.size();
            }
        }
    }

    std::cout << "2D has size " << scene_config.texture2D2Idx.size() << " and cube has size " << scene_config.textureCube2Idx.size() << std::endl;

    // then resize the texture2D and textureCube
    int tex2DCount = scene_config.texture2D2Idx.size();
    int texCubeCount = scene_config.textureCube2Idx.size();

    texture2DImages.resize(tex2DCount);
    texture2DImageMemorys.resize(tex2DCount);
    texture2DImageViews.resize(tex2DCount);

    textureCubeImages.resize(texCubeCount);
    textureCubeImageMemorys.resize(texCubeCount);
    textureCubeImageViews.resize(texCubeCount);
}


void generateSinglePixel(std::vector<double>& val, std::string& file_name) {
    stbi_uc* pixels = new stbi_uc[4];
    pixels[0] = static_cast<stbi_uc>(val[0] * 255);
    pixels[1] = static_cast<stbi_uc>(val[1] * 255);
    pixels[2] = static_cast<stbi_uc>(val[2] * 255);
    pixels[3] = 128;

    stbi_write_png(file_name.c_str(), 1, 1, 4, pixels, 0);
    delete[] pixels;
}