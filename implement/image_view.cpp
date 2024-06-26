#include "../scene_viewer.hpp"

void SceneViewer::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView2D(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);        
    }
}

VkImageView SceneViewer::createImageView2D(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

VkImageView SceneViewer::createImageViewCube(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 6,
        },
    };

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

void SceneViewer::createImage(uint32_t width, uint32_t height, VkImageCreateFlags flags, VkImageType imageType,
    VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage& image, VkDeviceMemory& imageMemory, int layers) {
    
    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = flags,
        .imageType = imageType,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = static_cast<uint32_t>(layers),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
    };

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void SceneViewer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding sampler2DLayoutBinding {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = MAX_INSTANCE,     
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding samplerCubeLayoutBinding {
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = MAX_INSTANCE,      
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    // first is the Light Source UBO
    VkDescriptorSetLayoutBinding fragLightUBOLayoutBinding {
        .binding = 3,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,      
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
    // then is the shadow map sampler2D
    VkDescriptorSetLayoutBinding shadowMapLayoutBinding {
        .binding = 4,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = MAX_LIGHT,      
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
    VkDescriptorSetLayoutBinding shadowMapCubeLayoutBinding {
        .binding = 5,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = MAX_LIGHT,      
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 6> bindings = { uboLayoutBinding, sampler2DLayoutBinding,
        samplerCubeLayoutBinding, fragLightUBOLayoutBinding, shadowMapLayoutBinding, shadowMapCubeLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void SceneViewer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void SceneViewer::updateUniformBuffer(uint32_t currentImage) {

    UniformBufferObject ubo{};

    // assign cameraPos
    ubo.cameraPos = scene_config.cameras[scene_config.cur_camera]->position;
    
    // ubo.model = cglm::rotate({0.0f, 1.0f, 0.0f}, time * cglm::to_radians(0.0f));
    ubo.model = cglm::identity(1.0f);

    std::shared_ptr<sconfig::Camera> camera = scene_config.cameras[scene_config.cur_camera];
    // ubo's view is based on current camera
    cglm::Vec3f camera_pos = camera->position;
    cglm::Vec3f view_point = camera->position + camera->dir;
    ubo.view = cglm::lookAt(camera_pos, view_point, camera->up);

    // ubo.view[0][0] = 0;
    // ubo.view[0][1] = 0;
    // ubo.view[0][2] = 1;
    // ubo.view[0][3] = 0;
    // ubo.view[1][0] = 1;
    // ubo.view[1][1] = 0;
    // ubo.view[1][2] = 0;
    // ubo.view[1][3] = 0;
    // ubo.view[2][0] = 0;
    // ubo.view[2][1] = 1;
    // ubo.view[2][2] = 0;
    // ubo.view[2][3] = 0;
    // ubo.view[3][0] = 0;
    // ubo.view[3][1] = -5;
    // ubo.view[3][2] = -12;
    // ubo.view[3][3] = 1;

    // ubo's projection is based on current camera
    ubo.proj = cglm::perspective(camera->vfov, camera->aspect, camera->near, camera->far);
    ubo.proj[1][1] *= -1;

    // for (int k=0; k<4; k++) {
    //     for (int j=0; j<4; j++) {
    //         ubo.proj[k][j] = 0;
    //     }
    // }
    // ubo.proj[0][0] = 0.57735f;
    // ubo.proj[1][1] = -0.57735f;
    // ubo.proj[2][2] = -1.0f;
    // ubo.proj[2][3] = -1.0f;
    // ubo.proj[3][2] = -0.2f;

    // ubo.view = cglm::lookAt(cglm::Vec3f(2.0f, 2.0f, 2.0f), cglm::Vec3f(0.0f, 0.0f, 0.0f), cglm::Vec3f(0.0f, 0.0f, 1.0f));
    // ubo.proj = cglm::perspective(cglm::to_radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    // ubo.proj[1][1] *= -1;

    int idx = 0;

    // now instead, we have great frame_material_meshInnerId2ModelMatrices
    auto& cur_frame_material_meshInnerId2ModelMatrices = frame_material_meshInnerId2ModelMatrices[currentFrame];
    for (auto& pair : cur_frame_material_meshInnerId2ModelMatrices) {
        // here, I just need to add matrices inorder. Draw takes care of realthings
        auto& meshId2ModelMatrices = pair.second;
        for (auto& p : meshId2ModelMatrices) {
            for (auto& model_matrix : p.second) {
                ubo.instanceModels[idx++] = model_matrix;
            }
        }
    }


    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void SceneViewer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 6> poolSizes{};
    poolSizes[0] = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    };
    poolSizes[1] = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    };
    poolSizes[2] = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    };
    poolSizes[3] = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    };
    poolSizes[4] = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    };
    poolSizes[5] = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    };

    VkDescriptorPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void SceneViewer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data(),
    };

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo {
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };
        VkDescriptorBufferInfo bufferInfo2 {
            .buffer = shadowUniformBuffers[i],
            .offset = 0,
            .range = sizeof(LightUniformBufferObject),
        };

        std::vector<VkDescriptorImageInfo> image2DInfos(MAX_INSTANCE);
        for (int j=0; j<texture2DImageViews.size(); j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = textureSampler2D,
                .imageView = texture2DImageViews[j],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            image2DInfos[j] = imageInfo;
        }
        for (int j=texture2DImageViews.size(); j<MAX_INSTANCE; j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = textureSampler2D,
                .imageView = texture2DImageViews[0],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            image2DInfos[j] = imageInfo;
        }

        std::vector<VkDescriptorImageInfo> imageCubeInfos(MAX_INSTANCE);
        for (int j=0; j<textureCubeImageViews.size(); j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = textureSamplerCube,
                .imageView = textureCubeImageViews[j],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            imageCubeInfos[j] = imageInfo;
        }
        for (int j=textureCubeImageViews.size(); j<MAX_INSTANCE; j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = textureSamplerCube,
                .imageView = textureCubeImageViews[0],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            imageCubeInfos[j] = imageInfo;
        }

        std::vector<VkDescriptorImageInfo> shadow2DInfos(MAX_LIGHT);
        for (int j=0; j<shadowMapImageViews.size(); j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = shadowMapSampler,
                .imageView = shadowMapImageViews[j],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            shadow2DInfos[j] = imageInfo;
        }
        for (int j=shadowMapImageViews.size(); j<MAX_LIGHT; j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = shadowMapSampler,
                .imageView = shadowMapImageViews[0],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            shadow2DInfos[j] = imageInfo;
        }

        std::vector<VkDescriptorImageInfo> shadowCubeInfos(MAX_LIGHT);
        for (int j = 0; j < shadowMapCubeImageViews.size(); j++) {
            VkDescriptorImageInfo imageInfo{
                .sampler = shadowMapCubeSampler,
                .imageView = shadowMapCubeImageViews[j],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            shadowCubeInfos[j] = imageInfo;
        }
        for (int j=shadowMapCubeImageViews.size(); j<MAX_LIGHT; j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = shadowMapCubeSampler,
                .imageView = shadowMapCubeImageViews[0],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            shadowCubeInfos[j] = imageInfo;
        }

        // descriptor WRITEs
        std::array<VkWriteDescriptorSet, 6> descriptorWrites{};

        descriptorWrites[0] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo,
        };

        descriptorWrites[1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = MAX_INSTANCE,   
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = image2DInfos.data(),   
        };

        descriptorWrites[2] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = MAX_INSTANCE,   
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = imageCubeInfos.data(),   
        };

        descriptorWrites[3] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo2, 
        };
        descriptorWrites[4] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 4,
            .dstArrayElement = 0,
            .descriptorCount = MAX_LIGHT,   
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = shadow2DInfos.data(),   
        };
        descriptorWrites[5] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 5,
            .dstArrayElement = 0,
            .descriptorCount = MAX_LIGHT,   
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = shadowCubeInfos.data(),   
        };

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}


