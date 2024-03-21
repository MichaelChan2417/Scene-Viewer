#include "../scene_viewer.hpp"

void SceneViewer::lightSetup() {
    shadowMapImages.resize(MAX_FRAMES_IN_FLIGHT);
    shadowMapImageViews.resize(MAX_FRAMES_IN_FLIGHT);
    shadowMapImageMemorys.resize(MAX_FRAMES_IN_FLIGHT);
    shadowMapFramebuffers.resize(MAX_FRAMES_IN_FLIGHT);

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
void SceneViewer::singleShadowRenderPass(VkCommandBuffer commandBuffer, int idx, int light_id) {
    std::shared_ptr<sconfig::Light> light = scene_config.id2lights[light_id];

    uint32_t shadow_width = static_cast<uint32_t>(light->shadow);
    VkExtent2D shadowExtent = { shadow_width, shadow_width };

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = shadowRenderPass,
        .framebuffer = shadowMapFramebuffers[idx],
        .renderArea = {
            .offset = {0, 0},
            .extent = shadowExtent,
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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


void SceneViewer::updateLightUniformBuffer(uint32_t currentImage, int idx, int light_id) {
    LightUniformBufferObject lubo{};

    std::shared_ptr<sconfig::Light> clight = scene_config.id2lights[light_id];
    int tidx = 0;
    for (auto& [id, light] : scene_config.id2lights) {

        // Spot Case
        if (std::holds_alternative<sconfig::Spot>(clight->data)) {
            sconfig::Spot spot_data = std::get<sconfig::Spot>(clight->data);

            lubo.lightPos[tidx] = cglm::Vec4f(light->position, 1.0f);
            lubo.lightDir[tidx] = cglm::Vec4f(light->direction, 0.0f);
            lubo.lightColor[tidx] = cglm::Vec4f(light->tint[0], light->tint[1], light->tint[2], 1.0f);

            // std::cout << "Light Position: " << light->position << std::endl;
            // std::cout << "Light Direction: " << light->direction << std::endl;
            // std::cout << "Light Up: " << light->up << std::endl;

            cglm::Vec3f view_point = light->position + light->direction;
            cglm::Mat44f view_mat = cglm::lookAt(light->position, view_point, light->up);
            lubo.lightViewMatrix[tidx] = view_mat;

            // for (int k = 0; k < 4; k++) {
            //     for (int l = 0; l < 4; l++) {
            //         std::cout << view_mat[k][l] << " ";
            //     }
            //     std::cout << std::endl;
            // }
            //     std::cout << "PROJ" << std::endl;

            cglm::Mat44f proj_mat = cglm::perspective(spot_data.fov, 1.0f, 0.1f, spot_data.limit);
            proj_mat[1][1] *= -1;
            lubo.lightProjMatrix[tidx] = proj_mat;

            // for (int k = 0; k < 4; k++) {
            //     for (int l = 0; l < 4; l++) {
            //         std::cout << proj_mat[k][l] << " ";
            //     }
            //     std::cout << std::endl;
            // }
            //     std::cout << std::endl;

            float radius = spot_data.radius;
            float fov = spot_data.fov;
            float limit = spot_data.limit;
            float blend = spot_data.blend;
            lubo.metadata1[tidx] = cglm::Vec4f(radius, fov/2.0f, limit, blend);

        }

    
        ++tidx;
    }
    
    lubo.metadata2[0][0] = static_cast<float>(0);

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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &shadowMapSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadowMap sampler!");
    }
}


void SceneViewer::createLightImagewithViews() {
    int light_count = scene_config.id2lights.size();
    shadowMapImages.resize(light_count);
    shadowMapImageViews.resize(light_count);
    shadowMapImageMemorys.resize(light_count);

    int idx = 0;
    for (auto& [id, light] : scene_config.id2lights) {
        createImage(light->shadow, light->shadow, 0, VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowMapImages[idx], shadowMapImageMemorys[idx], 1);
        shadowMapImageViews[idx] = createImageView2D(shadowMapImages[idx], VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
        ++idx;
    }
}


void SceneViewer::createLightFrameBuffers() {
    int light_count = scene_config.id2lights.size();
    shadowMapFramebuffers.resize(light_count);

    int idx = 0;
    for (auto& [id, light] : scene_config.id2lights) {
        std::array<VkImageView, 2> attachments = {
            shadowMapImageViews[idx],
            depthImageView
        };

        // std::cout << idx << std::endl;
        // I need to get
        VkFramebufferCreateInfo framebufferInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = shadowRenderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = static_cast<uint32_t>(light->shadow),
            .height = static_cast<uint32_t>(light->shadow),
            .layers = 1,
        };

        // std::cout << "pass" << std::endl;
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &shadowMapFramebuffers[idx]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
        ++idx;
        // std::cout << "pass2" << std::endl;
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,        // set layout to 1, for ubo descriptor
        .pSetLayouts = &shadowDescriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
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
