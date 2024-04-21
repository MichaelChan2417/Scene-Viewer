#include "../scene_viewer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void generateSinglePixel(const std::vector<double>& val, std::string& file_name);

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
    // std::cout << "texture image size: " << texWidth << "x" << texHeight << std::endl;

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

    // std::cout << "texture image size: " << texWidth << "x" << texHeight << std::endl;

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

    // then for cube
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSamplerCube) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}


void SceneViewer::texturePrepare() {
    std::cout << "Preparing textures..." << std::endl;
    // first is environment
    std::shared_ptr<sconfig::Environment> env = scene_config.environment;
    // env could be nullptr
    if (env != nullptr) {
        scene_config.textureCube2Idx[env->texture_src] = scene_config.textureCube2Idx.size();
    }

    // then is a environment light sampler
    scene_config.textureCube2Idx["textures/out.lambertian.png"] = scene_config.textureCube2Idx.size();

    // then for pbr 
    // TODO: note I cancel out the pbr values here
    // for (int i = 0; i <= 10; i++) {
    //     scene_config.textureCube2Idx["textures/pbr/out.ggx." + std::to_string(i) + ".png"] = scene_config.textureCube2Idx.size();
    // }
    // // also with lut
    // scene_config.texture2D2Idx["textures/pbr/out.lut.png"] = scene_config.texture2D2Idx.size();

    // default color
    scene_config.texture2D2Idx["textures/default2D.png"] = scene_config.texture2D2Idx.size();

    // then for all materials => load texture
    for (auto& [id, mat] : scene_config.id2material) {
        // if anymaterial has a normal map, then load it
        if (mat->normal_map != "") {
            if (scene_config.texture2D2Idx.find(mat->normal_map) != scene_config.texture2D2Idx.end()) {
                continue;
            }
            scene_config.texture2D2Idx[mat->normal_map] = scene_config.texture2D2Idx.size();
        }

        // lambertian - case
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

        // pbr - case
        if (mat->matetial_type == MaterialType::pbr) {
            std::shared_ptr<sconfig::Pbr> detail = std::get<std::shared_ptr<sconfig::Pbr>>(mat->matetial_detail);

            if (std::holds_alternative<std::vector<double>>(detail->albedo)) {
                std::string temp_file_name = "temp\\pbr_" + std::to_string(id) + "_albedo.png";
                generateSinglePixel(std::get<std::vector<double>>(detail->albedo), temp_file_name);
                detail->albedo = temp_file_name;
                detail->albedo_type = TextureType::texture2D;
            }
            std::string filename = std::get<std::string>(detail->albedo);
            if (detail->albedo_type == TextureType::texture2D) {
                if (scene_config.texture2D2Idx.find(filename) == scene_config.texture2D2Idx.end()) {
                    scene_config.texture2D2Idx[filename] = scene_config.texture2D2Idx.size();
                }
            }
            else {
                if (scene_config.textureCube2Idx.find(filename) == scene_config.textureCube2Idx.end()) {
                    scene_config.textureCube2Idx[filename] = scene_config.textureCube2Idx.size();
                }
            }

            if (std::holds_alternative<double>(detail->roughness)) {
                std::string temp_file_name = "temp\\pbr_" + std::to_string(id) + "_roughness.png";
                std::vector<double> val = { std::get<double>(detail->roughness), 0, 0 };
                generateSinglePixel(val, temp_file_name);
                detail->roughness = temp_file_name;
                detail->roughness_type = TextureType::texture2D;
            }
            filename = std::get<std::string>(detail->roughness);
            if (detail->roughness_type == TextureType::texture2D) {
                if (scene_config.texture2D2Idx.find(filename) == scene_config.texture2D2Idx.end()) {
                    scene_config.texture2D2Idx[filename] = scene_config.texture2D2Idx.size();
                }
            }
            else {
                if (scene_config.textureCube2Idx.find(filename) == scene_config.textureCube2Idx.end()) {
                    scene_config.textureCube2Idx[filename] = scene_config.textureCube2Idx.size();
                }
            }

            if (std::holds_alternative<double>(detail->metalness)) {
                std::string temp_file_name = "temp\\pbr_" + std::to_string(id) + "_metalness.png";
                std::vector<double> val = { std::get<double>(detail->metalness), 0, 0 };
                generateSinglePixel(val, temp_file_name);
                detail->metalness = temp_file_name;
                detail->metalness_type = TextureType::texture2D;
            }
            filename = std::get<std::string>(detail->metalness);
            if (detail->metalness_type == TextureType::texture2D) {
                if (scene_config.texture2D2Idx.find(filename) == scene_config.texture2D2Idx.end()) {
                    scene_config.texture2D2Idx[filename] = scene_config.texture2D2Idx.size();
                }
            }
            else {
                if (scene_config.textureCube2Idx.find(filename) == scene_config.textureCube2Idx.end()) {
                    scene_config.textureCube2Idx[filename] = scene_config.textureCube2Idx.size();
                }
            }
        }
    }

    // std::cout << "2D has size " << scene_config.texture2D2Idx.size() << " and cube has size " << scene_config.textureCube2Idx.size() << std::endl;

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


void generateSinglePixel(const std::vector<double>& val, std::string& file_name) {
    stbi_uc* pixels = new stbi_uc[4];
    pixels[0] = static_cast<stbi_uc>(val[0] * 255);
    pixels[1] = static_cast<stbi_uc>(val[1] * 255);
    pixels[2] = static_cast<stbi_uc>(val[2] * 255);
    pixels[3] = 128;

    stbi_write_png(file_name.c_str(), 1, 1, 4, pixels, 0);
    delete[] pixels;
}



void SceneViewer::createCloudNoiseImageWithView() {
    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_3D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .extent = {128, 128, 128},  // TODO: this is for specific noise
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (vkCreateImage(device, &imageInfo, nullptr, &cloudNoiseImage) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Cloud Noise image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, cloudNoiseImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    if (vkAllocateMemory(device, &allocInfo, nullptr, &cloudNoiseImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, cloudNoiseImage, cloudNoiseImageMemory, 0);

    // COPY IMAGE DATAS
    uint8_t* pdata = new uint8_t[128 * 128 * 128 * 4];

    auto tStart = std::chrono::high_resolution_clock::now();

    // referrencing vulkan example for parrallel
#pragma omp parallel for
    for (int k = 0; k < 128; k++) {
        std::string filename = "resources/cloud/noises/noise_" + std::to_string(k) + ".png";
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Failed to load cloud noise image at " + filename);
        }

        for (int j = 0; j < 128; j++) {
            for (int i = 0; i < 128; i++) {
                int srcIndex = (j * 128 + i) * 4;
                int destIndex = (k * 128 * 128 + j * 128 + i) * 4;
                for (int c = 0; c < 4; c++) {
                    pdata[destIndex + c] = pixels[srcIndex + c];
                }
            }
        }
    }
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

    std::cout << "Done in " << tDiff << "ms" << std::endl;

    
    // Use stage Buffer to copy
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize imageSize = 128 * 128 * 128 * 4;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pdata, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);


    // Copy to image
    transitionImageLayout(cloudNoiseImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

    // ======================= Begin Single Time Command =======================
    VkCommandBuffer ccommandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy bufferCopyRegion{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {128, 128, 128},
    };

    vkCmdCopyBufferToImage(ccommandBuffer, stagingBuffer, cloudNoiseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

    endSingleTimeCommands(ccommandBuffer);
    // ======================= End Single Time Command =======================

    transitionImageLayout(cloudNoiseImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);


    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);


    // Then create image view
    VkImageViewCreateInfo viewInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = cloudNoiseImage,
        .viewType = VK_IMAGE_VIEW_TYPE_3D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    if (vkCreateImageView(device, &viewInfo, nullptr, &cloudNoiseImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}


void SceneViewer::createCloudImages(const std::string& filename, int idx) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    std::cout << "cloud image size: " << texWidth << "x" << texHeight << std::endl;

    if (!pixels) {
        throw std::runtime_error("Failed to load cloud image!");
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
        cloudImages[idx], cloudImageMemorys[idx], 1);

    transitionImageLayout(cloudImages[idx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        copyBufferToImage(stagingBuffer, cloudImages[idx], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
    transitionImageLayout(cloudImages[idx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}