#include "../scene_viewer.hpp"

void SceneViewer::createCloudPipeline() {
    std::vector<char> vertShaderCode = readFile("shaders/cloud/vert.spv");
    std::vector<char> fragShaderCode = readFile("shaders/cloud/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // defining module creation info
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

    // vertex input
    auto bindingDescription = CloudVertex::getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = CloudVertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };

    // input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // viewport state
    VkPipelineViewportStateCreateInfo viewportState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    // rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,              // backside culling
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        // .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    // multisampling
    VkPipelineMultisampleStateCreateInfo multisampling {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    // depth and stencil testing
    VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    // color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
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

    // dynamic state
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,        // set layout to 1, for ubo descriptor
        .pSetLayouts = &cloudDescriptorSetLayout,  
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &cloudPipelineLayout) != VK_SUCCESS) {
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
        .layout = cloudPipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &cloudPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create cloud graphics pipeline!");
    }

    // clean up
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void SceneViewer::createCloudDescriptorSetLayout() {
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
        .descriptorCount = 32,     
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding samplerNoiseLayoutBinding {
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,     
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding fragUboLayoutBinding {
        .binding = 3,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };


    std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, sampler2DLayoutBinding,
    samplerNoiseLayoutBinding, fragUboLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &cloudDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void SceneViewer::createCloudDescriptorPool() {
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
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

    VkDescriptorPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &cloudDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create cloud descriptor pool!");
    }
}

void SceneViewer::createCloudDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, cloudDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = cloudDescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data(),
    };

    cloudDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, cloudDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo {
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };

        std::vector<VkDescriptorImageInfo> image2DInfos(32);
        for (int j=0; j<32; j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = cloudSampler,
                .imageView = cloudImageViews[j],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            image2DInfos[j] = imageInfo;
        }

        std::vector<VkDescriptorImageInfo> imageNoiseInfos(1);
        for (int j=0; j<1; j++) {
            VkDescriptorImageInfo imageInfo {
                .sampler = cloudNoiseSampler,
                .imageView = cloudNoiseImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            imageNoiseInfos[j] = imageInfo;
        }
        
        VkDescriptorBufferInfo bufferInfo2 {
            .buffer = cloudUniformBuffers[i],
            .offset = 0,
            .range = sizeof(CloudUniformBufferObject),
        };


        // descriptor WRITEs
        std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

        descriptorWrites[0] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = cloudDescriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo,
        };

        descriptorWrites[1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = cloudDescriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 32,   
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = image2DInfos.data(),   
        };

        descriptorWrites[2] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = cloudDescriptorSets[i],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,   
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = imageNoiseInfos.data(),   
        };

        descriptorWrites[3] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = cloudDescriptorSets[i],
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo2,
        };

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}

void SceneViewer::createCloudSampler() {
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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &cloudSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    if (vkCreateSampler(device, &samplerInfo, nullptr, &cloudNoiseSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void SceneViewer::createCloudImagesWithViews() {
    std::vector<std::string> cloudImagePaths;
    for (int i=0; i<32; i++) {
        cloudImagePaths.push_back("resources/cloud/models/model_" + std::to_string(i) + ".png");
    }

    cloudImages.resize(cloudImagePaths.size());
    cloudImageMemorys.resize(cloudImagePaths.size());
    cloudImageViews.resize(cloudImagePaths.size());

    for (size_t i = 0; i < cloudImagePaths.size(); i++) {
        createCloudImages(cloudImagePaths[i], i);
        cloudImageViews[i] = createImageView2D(cloudImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }


    // then for noises
    createCloudNoiseImageWithView();
}

void SceneViewer::createCloudUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(CloudUniformBufferObject);

    cloudUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    cloudUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    cloudUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, cloudUniformBuffers[i], cloudUniformBuffersMemory[i]);
        vkMapMemory(device, cloudUniformBuffersMemory[i], 0, bufferSize, 0, &cloudUniformBuffersMapped[i]);
    }
}

cglm::Vec4f getCurLightPos(double dtime) {
    float x = 25.0 * cos(dtime * 3.1415926 / 8);
    float y = 25.0 * sin(dtime * 3.1415926 / 8);
    float z = 18.0;
    return cglm::Vec4f(x, y, z, 1.0);
}

cglm::Vec4f getCurLightColor(double dtime) {
    // module dtime to 4
    dtime = fmod(dtime, 8.0);
    cglm::Vec4f color1(1.0, 0.5, 0.0, 1.0);
    cglm::Vec4f color2(0.85, 0.85, 1.0, 1.0);

    if (dtime < 2.0) {
        return color1 * (2.0 - dtime) / 2.0 + color2 * dtime / 2.0;
    }
    else if (dtime > 6.0) {
        return color1 * (dtime - 6.0) / 2.0 + color2 * (8.0 - dtime) / 2.0;
    }
    else {
        return cglm::Vec4f(0.85, 0.85, 1.0, 1.0);
    }
}

void SceneViewer::updateCloudUniformBuffer(uint32_t currentImage) {
    CloudUniformBufferObject cubo{};

    auto currentTime = std::chrono::high_resolution_clock::now();
    double dtime = std::chrono::duration<double, std::chrono::seconds::period>(currentTime - startTime).count();

    // cubo.lightPos = getCurLightPos(dtime);
    cubo.lightPos = cglm::Vec4f(25, 25, 14, 1.0);
    // cubo.lightColor = getCurLightColor(dtime);
    cubo.lightColor = cglm::Vec4f(1.0, 0.85, 0.85, 1.0);
    cubo.metadata = cglm::Vec4f(dtime, dtime, 0.0, 0.0);

    memcpy(cloudUniformBuffersMapped[currentImage], &cubo, sizeof(cubo));
}


void SceneViewer::cleanCloudResources() {

    vkDestroyImageView(device, cloudNoiseImageView, nullptr);
    vkDestroyImage(device, cloudNoiseImage, nullptr);
    vkFreeMemory(device, cloudNoiseImageMemory, nullptr);

    int cloudImageSize = cloudImageViews.size();
    for (int i=0; i<cloudImageSize; i++) {
        vkDestroyImageView(device, cloudImageViews[i], nullptr);
        vkDestroyImage(device, cloudImages[i], nullptr);
        vkFreeMemory(device, cloudImageMemorys[i], nullptr);
    }

    // remove cloudSampler
    vkDestroySampler(device, cloudSampler, nullptr);
    vkDestroySampler(device, cloudNoiseSampler, nullptr);

    // remove pipeline
    vkDestroyPipeline(device, cloudPipeline, nullptr);
    vkDestroyPipelineLayout(device, cloudPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, cloudDescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, cloudDescriptorPool, nullptr);

    // remove cloudUniformBuffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, cloudUniformBuffers[i], nullptr);
        vkFreeMemory(device, cloudUniformBuffersMemory[i], nullptr);
    }

    // remove cloudIndexBuffers
    for (int i = 0; i < scene_config.id2clouds.size(); i++) {
        vkDestroyBuffer(device, cloudVertexBuffers[i], nullptr);
        vkFreeMemory(device, cloudVertexBufferMemorys[i], nullptr);
    }
    vkDestroyBuffer(device, cloudIndexBuffer, nullptr);
    vkFreeMemory(device, cloudIndexBufferMemory, nullptr);
}