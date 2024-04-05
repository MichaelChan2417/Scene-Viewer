#include "../scene_viewer.hpp"

struct alignas(16) PushConstantStruct {
    int cur_idx;
    alignas(16) cglm::Mat44f view;
    alignas(16) cglm::Mat44f proj;
};

void SceneViewer::lightSetup() {
    // shadowMapImages.resize(MAX_FRAMES_IN_FLIGHT);
    // shadowMapImageViews.resize(MAX_FRAMES_IN_FLIGHT);
    // shadowMapImageMemorys.resize(MAX_FRAMES_IN_FLIGHT);
    // shadowMapFramebuffers.resize(MAX_FRAMES_IN_FLIGHT);

    // referrencing Vulkan Tutorial: using 2 ways in image_view to generate cubemap
    // using 1 image, described in 2 imageviews, one for generating, onr for sampling
    // https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingomni/shadowmappingomni.cpp
    // shadowMapCubeImages.resize(MAX_FRAMES_IN_FLIGHT);
    // shadowMapCubeImageViews.resize(MAX_FRAMES_IN_FLIGHT);
    // shadowMapCubeImageMemorys.resize(MAX_FRAMES_IN_FLIGHT);
    // shadowMapCubeFacesImageViews.resize(MAX_FRAMES_IN_FLIGHT);
    // shadowMapCubeFramebuffers.resize(MAX_FRAMES_IN_FLIGHT);

    // for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //     shadowMapCubeFacesImageViews.resize(6);
    //     shadowMapCubeFramebuffers.resize(6);
    // }

    std::cout << "i1" << std::endl;
    createRenderPass(VK_FORMAT_R32G32B32A32_SFLOAT, shadowRenderPass, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    std::cout << "i2" << std::endl;
    createShadowDescriptorSetLayout();
    std::cout << "i3" << std::endl;
    createShadowGraphicsPipeline();
    std::cout << "i4" << std::endl;
    createLightImagewithViews();
    std::cout << "i5" << std::endl;
    createLightFrameBuffers();
    std::cout << "i6" << std::endl;
    createShadowMapSampler();
    std::cout << "i7" << std::endl;
    createLightUniformBuffers();
    std::cout << "i8" << std::endl;
    createLightDescriptorPool();
    std::cout << "i9" << std::endl;
    createLightDescriptorSets();
    std::cout << "i10" << std::endl;
}


void SceneViewer::createLightResources() {
    lightSetup();
}


// ---------------------------------------- Shadow Vulkan Resources ----------------------------------------
void SceneViewer::singleCubeShadowRenderPass(VkCommandBuffer commandBuffer, int spot_idx, int sphere_idx, int light_id) {
    std::shared_ptr<sconfig::Light> light = scene_config.id2lights[light_id];
    sconfig::Sphere sphere_data = std::get<sconfig::Sphere>(light->data);
    uint32_t shadow_width = static_cast<uint32_t>(light->shadow);
    uint32_t shadow_height = shadow_width;
    VkExtent2D shadowExtent = { shadow_width, shadow_height };

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    for (int j = 0; j < 6; j++) {
        // on face-j
        VkRenderPassBeginInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = shadowRenderPass,
            .framebuffer = shadowMapCubeFramebuffers[sphere_idx][j],
            .renderArea = {
                .offset = {0, 0},
                .extent = shadowExtent,
            },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data(),
        };

        // get view matrix
        PushConstantStruct pushConstantStruct{};
        cglm::Mat44f viewMat = cglm::identity(1.0f);
        switch (j) {
        case 0: {
                // viewMat = cglm::rotate(viewMat, cglm::to_radians(90.0f), cglm::Vec3f(0.0f, 1.0f, 0.0f));
                // viewMat = cglm::rotate(viewMat, cglm::to_radians(180.0f), cglm::Vec3f(1.0f, 0.0f, 0.0f));
                viewMat = cglm::lookAtLeft(light->position, light->position + cglm::Vec3f(1.0f, 0.0f, 0.0f), cglm::Vec3f(0.0f, 1.0f, 0.0f));
                break;
            }
        case 1: {
                // viewMat = cglm::rotate(viewMat, cglm::to_radians(-90.0f), cglm::Vec3f(0.0f, 1.0f, 0.0f));
                // viewMat = cglm::rotate(viewMat, cglm::to_radians(180.0f), cglm::Vec3f(1.0f, 0.0f, 0.0f));
                viewMat = cglm::lookAtLeft(light->position, light->position + cglm::Vec3f(-1.0f, 0.0f, 0.0f), cglm::Vec3f(0.0f, 1.0f, 0.0f));
                break;
            }
        case 2: {
                // viewMat = cglm::rotate(viewMat, cglm::to_radians(-90.0f), cglm::Vec3f(1.0f, 0.0f, 0.0f));
                viewMat = cglm::lookAtLeft(light->position, light->position + cglm::Vec3f(0.0f, 1.0f, 0.0f), cglm::Vec3f(0.0f, 0.0f, -1.0f));
                break;
            }
        case 3: {
                // viewMat = cglm::rotate(viewMat, cglm::to_radians(90.0f), cglm::Vec3f(1.0f, 0.0f, 0.0f));
                viewMat = cglm::lookAtLeft(light->position, light->position + cglm::Vec3f(0.0f, -1.0f, 0.0f), cglm::Vec3f(0.0f, 0.0f, 1.0f));
                break;
            }
        case 4: {
                // viewMat = cglm::rotate(viewMat, cglm::to_radians(180.0f), cglm::Vec3f(1.0f, 0.0f, 0.0f));
                viewMat = cglm::lookAtLeft(light->position, light->position + cglm::Vec3f(0.0f, 0.0f, 1.0f), cglm::Vec3f(0.0f, 1.0f, 0.0f));
                break;
            }
        case 5: {
                // viewMat = cglm::rotate(viewMat, cglm::to_radians(180.0f), cglm::Vec3f(0.0f, 0.0f, 1.0f));
                viewMat = cglm::lookAtLeft(light->position, light->position + cglm::Vec3f(0.0f, 0.0f, -1.0f), cglm::Vec3f(0.0f, 1.0f, 0.0f));
                break;
            }
        }
        pushConstantStruct.cur_idx = sphere_idx + spot_idx;
        pushConstantStruct.view = viewMat;
        pushConstantStruct.proj = cglm::perspective(cglm::to_radians(90.0f), 1.0f, 0.1f, sphere_data.limit);
        pushConstantStruct.proj[1][1] *= -1;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdPushConstants(commandBuffer, shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantStruct), &pushConstantStruct);

        VkViewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(shadowExtent.width),
            .height = static_cast<float>(shadowExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = shadowExtent;
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = { 0 };

        auto& cur_frame_material_meshInnerId2ModelMatrices = frame_material_meshInnerId2ModelMatrices[currentFrame];
        int curInstanceIndex = 0;

        for (auto& pair : cur_frame_material_meshInnerId2ModelMatrices) {
            MaterialType materialType = pair.first;
            auto& meshInnerId2ModelMatrices = pair.second;
            VkPipeline graphicsPipeline = material2Pipelines[materialType];

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowGraphicsPipeline);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1, &shadowDescriptorSets[currentFrame], 0, nullptr);

            frameRealDraw(commandBuffer, curInstanceIndex, meshInnerId2ModelMatrices);
        }

        vkCmdEndRenderPass(commandBuffer);
    }
    
}

void SceneViewer::singleShadowRenderPass(VkCommandBuffer commandBuffer, int spot_idx, int sphere_idx, int light_id) {
    std::shared_ptr<sconfig::Light> light = scene_config.id2lights[light_id];

    uint32_t shadow_width = static_cast<uint32_t>(light->shadow);
    uint32_t shadow_height = shadow_width;
    VkExtent2D shadowExtent = { shadow_width, shadow_height };

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = shadowRenderPass,
        .framebuffer = shadowMapFramebuffers[spot_idx],
        .renderArea = {
            .offset = {0, 0},
            .extent = shadowExtent,
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    PushConstantStruct pushConstantStruct{};
    pushConstantStruct.cur_idx = sphere_idx + spot_idx;
    vkCmdPushConstants(commandBuffer, shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantStruct), &pushConstantStruct);

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(shadowExtent.width),
        .height = static_cast<float>(shadowExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = shadowExtent;
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = { 0 };

    auto& cur_frame_material_meshInnerId2ModelMatrices = frame_material_meshInnerId2ModelMatrices[currentFrame];
    int curInstanceIndex = 0;

    for (auto& pair : cur_frame_material_meshInnerId2ModelMatrices) {
        MaterialType materialType = pair.first;
        auto& meshInnerId2ModelMatrices = pair.second;
        VkPipeline graphicsPipeline = material2Pipelines[materialType];

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowGraphicsPipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1, &shadowDescriptorSets[currentFrame], 0, nullptr);

        frameRealDraw(commandBuffer, curInstanceIndex, meshInnerId2ModelMatrices);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void SceneViewer::updateWholeLightUniformBuffer(uint32_t currentImage, LightUniformBufferObject& lubo) {
    int idx = 0;
    for (auto& [id, light] : scene_config.id2lights) {
        if (light->type == sconfig::LightType::SPOT) {
            sconfig::Spot spot_data = std::get<sconfig::Spot>(light->data);
            lubo.lightPos[idx] = cglm::Vec4f(light->position, 0.0f);                    // last bit is type
            lubo.lightDir[idx] = cglm::Vec4f(light->direction, spot_data.power);        // last bit is power
            lubo.lightColor[idx] = cglm::Vec4f(light->tint[0], light->tint[1], light->tint[2], 1.0f);

            cglm::Vec3f view_point = light->position + light->direction;
            cglm::Mat44f view_mat = cglm::lookAt(light->position, view_point, light->up);
            lubo.lightViewMatrix[idx] = view_mat;

            cglm::Mat44f proj_mat = cglm::perspective(spot_data.fov, 1.0f, 0.1f, spot_data.limit);
            proj_mat[1][1] *= -1;
            lubo.lightProjMatrix[idx] = proj_mat;

            float radius = spot_data.radius;
            float fov = spot_data.fov;
            float limit = spot_data.limit;
            float blend = spot_data.blend;
            lubo.metadata1[idx] = cglm::Vec4f(radius, fov / 2.0f, limit, blend);
            lubo.metadata2[idx][3] = light->shadow;
        }
        else if (light->type == sconfig::LightType::SPHERE) {
            sconfig::Sphere sphere_data = std::get<sconfig::Sphere>(light->data);
            lubo.lightPos[idx] = cglm::Vec4f(light->position, 1.0f);                     // last bit is type
            lubo.lightDir[idx] = cglm::Vec4f(0.0f, 0.0f, 0.0f, sphere_data.power);       // last bit is power
            lubo.lightColor[idx] = cglm::Vec4f(light->tint[0], light->tint[1], light->tint[2], 1.0f);

            float radius = sphere_data.radius;
            float limit = sphere_data.limit;

            lubo.metadata1[idx] = cglm::Vec4f(radius, limit, 0.0f, 0.0f);
            lubo.metadata2[idx][3] = light->shadow;
        }

        ++idx;
    }

    lubo.metadata2[0][2] = scene_config.id2lights.size();

    memcpy(shadowUniformBuffersMapped[currentImage], &lubo, sizeof(lubo));
}

void SceneViewer::updateCurLightUBOIndex(uint32_t currentImage, int idx, LightUniformBufferObject& lubo) {
    lubo.metadata2[0][0] = idx;
    std::cout << "Modified Index: " << idx << std::endl;
    memcpy(shadowUniformBuffersMapped[currentImage], &lubo, sizeof(lubo));
}

void SceneViewer::updateLightUniformBuffer(uint32_t currentImage, int idx, int light_id) {
    LightUniformBufferObject lubo{};

    std::shared_ptr<sconfig::Light> clight = scene_config.id2lights[light_id];
    int tidx = 0;
    for (auto& [id, light] : scene_config.id2lights) {

        lubo.lightColor[tidx] = cglm::Vec4f(light->tint[0], light->tint[1], light->tint[2], 1.0f);
        lubo.metadata2[tidx][3] = clight->shadow;

        // Spot Case
        if (std::holds_alternative<sconfig::Spot>(clight->data)) {
            sconfig::Spot spot_data = std::get<sconfig::Spot>(clight->data);

            lubo.lightPos[tidx] = cglm::Vec4f(light->position, 0.0f);       // last bit is type
            lubo.lightDir[tidx] = cglm::Vec4f(light->direction, spot_data.power); // last bit is power

            // std::cout << "Light Position: " << light->position << std::endl;
            // std::cout << "Light Direction: " << light->direction << std::endl;
            // std::cout << "Light Up: " << light->up << std::endl;

            cglm::Vec3f view_point = light->position + light->direction;
            cglm::Mat44f view_mat = cglm::lookAt(light->position, view_point, light->up);
            lubo.lightViewMatrix[tidx] = view_mat;

            cglm::Mat44f proj_mat = cglm::perspective(spot_data.fov, 1.0f, 0.1f, spot_data.limit);
            proj_mat[1][1] *= -1;
            lubo.lightProjMatrix[tidx] = proj_mat;

            float radius = spot_data.radius;
            float fov = spot_data.fov;
            float limit = spot_data.limit;
            float blend = spot_data.blend;
            lubo.metadata1[tidx] = cglm::Vec4f(radius, fov/2.0f, limit, blend);
        }

        else if (std::holds_alternative<sconfig::Sphere>(clight->data)) {
            sconfig::Sphere sphere_data = std::get<sconfig::Sphere>(clight->data);
            lubo.lightPos[tidx] = cglm::Vec4f(light->position, 1.0f);     // last bit is type
            lubo.lightDir[tidx] = cglm::Vec4f(light->direction, sphere_data.power); // last bit is power

            float radius = sphere_data.radius;
            float limit = sphere_data.limit;

            lubo.metadata1[tidx] = cglm::Vec4f(radius, limit, 0.0f, 0.0f);
        }

    
        ++tidx;
    }
    
    lubo.metadata2[0][0] = static_cast<float>(idx);

    memcpy(shadowUniformBuffersMapped[currentImage], &lubo, sizeof(lubo));
}


void SceneViewer::createLightDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, shadowDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = shadowDescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data(),
    };

    shadowDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, shadowDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate shadow descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo1 {
            .buffer = shadowUniformBuffers[i],
            .offset = 0,
            .range = sizeof(LightUniformBufferObject),
        };

        VkDescriptorBufferInfo bufferInfo2 {
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = shadowDescriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo1,
        };

        descriptorWrites[1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = shadowDescriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo2,
        };

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}


void SceneViewer::createLightDescriptorPool() {
    // this is only for shadow render pass
    std::array<VkDescriptorPoolSize, 2> poolSizes = {
        VkDescriptorPoolSize {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        },
        VkDescriptorPoolSize {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        }
    };

    VkDescriptorPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &shadowDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


void SceneViewer::createLightUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(LightUniformBufferObject);

    shadowUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    shadowUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    shadowUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, shadowUniformBuffers[i], shadowUniformBuffersMemory[i]);
        vkMapMemory(device, shadowUniformBuffersMemory[i], 0, bufferSize, 0, &shadowUniformBuffersMapped[i]);
    }
}


void SceneViewer::createShadowMapSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,   // changing to nearest
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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &shadowMapSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadowMap sampler!");
    }

    if (vkCreateSampler(device, &samplerInfo, nullptr, &shadowMapCubeSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadowCubeMap sampler!");
    }
}


void SceneViewer::createLightImagewithViews() {
    int spot_cnt = 0;
    int sphere_cnt = 0;

    for (auto& [id, light] : scene_config.id2lights) {
        if (light->type == sconfig::LightType::SPOT) {
            ++spot_cnt;
        }
        else if (light->type == sconfig::LightType::SPHERE) {
            ++sphere_cnt;
        }
    }

    std::cout << "Spot Light Count: " << spot_cnt << std::endl;
    std::cout << "Sphere Light Count: " << sphere_cnt << std::endl;
    shadowMapImages.resize(spot_cnt);
    shadowMapImageViews.resize(spot_cnt);
    shadowMapImageMemorys.resize(spot_cnt);
    shadowDepthImages.resize(spot_cnt);
    shadowDepthImageViews.resize(spot_cnt);
    shadowDepthImageMemorys.resize(spot_cnt);

    shadowMapCubeImages.resize(sphere_cnt);
    shadowMapCubeImageViews.resize(sphere_cnt);
    shadowMapCubeImageMemorys.resize(sphere_cnt);
    shadowMapCubeFacesImageViews.resize(sphere_cnt);
    for (int j=0; j<sphere_cnt; j++) {
        shadowMapCubeFacesImageViews[j].resize(6);
    }
    shadowCubeDepthImages.resize(sphere_cnt);
    shadowCubeDepthImageViews.resize(sphere_cnt);
    shadowCubeDepthImageMemorys.resize(sphere_cnt);

    int idx = 0;
    VkFormat depthFormat = findDepthFormat();
    for (auto& [id, light] : scene_config.id2lights) {
        // together with depth image
        if (light->type == sconfig::LightType::SPOT) {
            createImage(light->shadow, light->shadow, 0, VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowMapImages[idx], shadowMapImageMemorys[idx], 1);
            createImage(light->shadow, light->shadow, 0, VK_IMAGE_TYPE_2D, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowDepthImages[idx], shadowDepthImageMemorys[idx], 1);
            shadowMapImageViews[idx] = createImageView2D(shadowMapImages[idx], VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
            shadowDepthImageViews[idx] = createImageView2D(shadowDepthImages[idx], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
            ++idx;
        }

    }

    idx = 0;
    for (auto& [id, light] : scene_config.id2lights) {
        if (light->type == sconfig::LightType::SPHERE) {
            createImage(light->shadow, light->shadow, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                shadowMapCubeImages[idx], shadowMapCubeImageMemorys[idx], 6);
            createImage(light->shadow, light->shadow, 0, VK_IMAGE_TYPE_2D, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowCubeDepthImages[idx], shadowCubeDepthImageMemorys[idx], 1);

            VkImageViewCreateInfo viewInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = shadowMapCubeImages[idx],
                .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 6,
                },
            };

            // first is cube view
            if (vkCreateImageView(device, &viewInfo, nullptr, &shadowMapCubeImageViews[idx]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }

            // then is 6 faces 2D view
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.subresourceRange.layerCount = 1;
            for (int j = 0; j < 6; j++) {
                viewInfo.subresourceRange.baseArrayLayer = j;
                if (vkCreateImageView(device, &viewInfo, nullptr, &shadowMapCubeFacesImageViews[idx][j]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create texture image view!");
                }
            }

            // then depth view
            shadowCubeDepthImageViews[idx] = createImageView2D(shadowCubeDepthImages[idx], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

            ++idx;
        }
    }

}


void SceneViewer::createLightFrameBuffers() {
    int spot_cnt = 0;
    int sphere_cnt = 0;

    for (auto& [id, light] : scene_config.id2lights) {
        if (light->type == sconfig::LightType::SPOT) {
            ++spot_cnt;
        }
        else if (light->type == sconfig::LightType::SPHERE) {
            ++sphere_cnt;
        }
    }

    shadowMapFramebuffers.resize(spot_cnt);
    shadowMapCubeFramebuffers.resize(sphere_cnt);
    for (int i=0; i<sphere_cnt; i++) {
        shadowMapCubeFramebuffers[i].resize(6);
    }

    int idx = 0;
    // spot lights
    for (auto& [id, light] : scene_config.id2lights) {
        uint32_t l_width = static_cast<uint32_t>(light->shadow);
        uint32_t l_height = l_width;
        if (light->type != sconfig::LightType::SPOT) {
            continue;
        }

        std::array<VkImageView, 2> attachments = {
            shadowMapImageViews[idx],
            shadowDepthImageViews[idx]
        };

        // I need to get
        VkFramebufferCreateInfo framebufferInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = shadowRenderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = l_width,
            .height = l_height,
            .layers = 1,
        };

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &shadowMapFramebuffers[idx]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
        ++idx;
    }

    // sphere lights
    idx = 0;
    for (auto& [id, light] : scene_config.id2lights) {
        if (light->type != sconfig::LightType::SPHERE) {
            continue;
        }
        uint32_t l_width = static_cast<uint32_t>(light->shadow);

        VkImageView attachments[2];
        attachments[1] = shadowCubeDepthImageViews[idx];

        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = shadowRenderPass,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = l_width,
            .height = l_width,
            .layers = 1,
        };

        for (int j = 0; j < 6; j++) {
            attachments[0] = shadowMapCubeFacesImageViews[idx][j];
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &shadowMapCubeFramebuffers[idx][j]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create cube framebuffer!");
            }
        }

        ++idx;
    }
}

void SceneViewer::createShadowGraphicsPipeline() {
    // TODO: maybe update with different light type
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;

    vertShaderCode = readFile("shaders/shadow/vert.spv");
    fragShaderCode = readFile("shaders/shadow/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
    };
    VkPipelineShaderStageCreateInfo fragShaderStageInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main",
    };
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getSimpleAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };
    VkPipelineViewportStateCreateInfo viewportState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };
    VkPipelineRasterizationStateCreateInfo rasterizer {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,              // backside culling -> VK_CULL_MODE_FRONT_BIT
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        // .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };
    VkPipelineMultisampleStateCreateInfo multisampling {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };
    VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };
    VkPipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo colorBlending {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    VkPushConstantRange pushConstantRange{ 
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstantStruct),
    };
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,        // set layout to 1, for ubo descriptor
        .pSetLayouts = &shadowDescriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &shadowPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = shadowPipelineLayout,
        .renderPass = shadowRenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shadowGraphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // clean up
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}


void SceneViewer::createShadowDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding uboLayoutBinding2 {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, uboLayoutBinding2};
    
    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &shadowDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void SceneViewer::cleanShadowResources() {
    
    int spot_cnt = 0;
    int sphere_cnt = 0;

    for (auto& [id, light] : scene_config.id2lights) {
        if (light->type == sconfig::LightType::SPOT) {
            ++spot_cnt;
        }
        else if (light->type == sconfig::LightType::SPHERE) {
            ++sphere_cnt;
        }
    }

    for (size_t i = 0; i < spot_cnt; i++) {
        vkDestroyImageView(device, shadowDepthImageViews[i], nullptr);
        vkDestroyImage(device, shadowDepthImages[i], nullptr);
        vkFreeMemory(device, shadowDepthImageMemorys[i], nullptr);
    }
    for (auto shadowFrameBuffer : shadowMapFramebuffers) {
        vkDestroyFramebuffer(device, shadowFrameBuffer, nullptr);
    }

    for (size_t i = 0; i < sphere_cnt; i++) {
        for (int j = 0; j < 6; j++) {
            vkDestroyImageView(device, shadowMapCubeFacesImageViews[i][j], nullptr);
        }
        vkDestroyImageView(device, shadowCubeDepthImageViews[i], nullptr);
        vkDestroyImage(device, shadowCubeDepthImages[i], nullptr);
        vkFreeMemory(device, shadowCubeDepthImageMemorys[i], nullptr);
        vkDestroyImageView(device, shadowMapCubeImageViews[i], nullptr);
        vkDestroyImage(device, shadowMapCubeImages[i], nullptr);
        vkFreeMemory(device, shadowMapCubeImageMemorys[i], nullptr);
        for (int j = 0; j < 6; j++) {
            vkDestroyFramebuffer(device, shadowMapCubeFramebuffers[i][j], nullptr);
        }
    }
    
    vkDestroySampler(device, shadowMapSampler, nullptr);
    vkDestroySampler(device, shadowMapCubeSampler, nullptr);

    for (auto& imageView : shadowMapImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    for (auto& image : shadowMapImages) {
        vkDestroyImage(device, image, nullptr);
    }
    for (auto& memory : shadowMapImageMemorys) {
        vkFreeMemory(device, memory, nullptr);
    }

    // destroy shadow pipeline
    vkDestroyPipeline(device, shadowGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, shadowPipelineLayout, nullptr);
    vkDestroyRenderPass(device, shadowRenderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        vkDestroyBuffer(device, shadowUniformBuffers[i], nullptr);
        vkFreeMemory(device, shadowUniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, shadowDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, shadowDescriptorSetLayout, nullptr);
}