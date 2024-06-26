#include "../scene_viewer.hpp"

void SceneViewer::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layers = 1,
        };

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }    
}

void SceneViewer::createHeadlessFramebuffers() {
    
    createColorResources();
    createHeadlessDepthResources();

    VkImageView attachments[2];
    attachments[0] = colorImageView;
    attachments[1] = depthImageView;

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = static_cast<uint32_t>(window_width),
        .height = static_cast<uint32_t>(window_height),
        .layers = 1,
    };

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &headlessFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create framebuffer!");
    }
}

void SceneViewer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
    };
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void SceneViewer::createHeadlessCommandPool() {
    VkCommandPoolCreateInfo cmdPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex,
    };
    if (vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void SceneViewer::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void SceneViewer::createHeadlessCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    copyCmdBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo copyAllocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(copyCmdBuffers.size()),
    };
    if (vkAllocateCommandBuffers(device, &copyAllocInfo, copyCmdBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void SceneViewer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // begin recording command buffer
    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // first make the update, for both rendering use
    updateUniformBuffer(currentFrame);

    // begin drawing, (render pass)
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.52f, 0.8f, 0.92f, 1.0f}};
    // clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = { 1.0f, 0 };

    // instead, I begin a render pass for shadow map
    LightUniformBufferObject lubo{};
    updateWholeLightUniformBuffer(currentFrame, lubo);

    int spot_idx = 0, sphere_idx = 0;
    for (auto& [id, light] : scene_config.id2lights) {
        // updateCurLightUBOIndex(currentFrame, spot_idx + sphere_idx, lubo);
        if (light->type == sconfig::LightType::SPOT) {
            singleShadowRenderPass(commandBuffer, spot_idx, sphere_idx, id);
            ++spot_idx;
        } else if (light->type == sconfig::LightType::SPHERE) {
            singleCubeShadowRenderPass(commandBuffer, spot_idx, sphere_idx, id);
            ++sphere_idx;
        }
    }
    
    // std::cout << lidx << std::endl;
    /*
        Learning from vulkan example: this is important!
        Note: Explicit synchronization is not required between the render pass,
        as this is done implicit via sub pass dependencies
    */
    
    // content drawing
    // updateUniformBuffer(currentFrame);
    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = swapChainFramebuffers[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = swapChainExtent,
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    
    double windowAspect = static_cast<double>(window_width) / static_cast<double>(window_height);
    std::string cur_camera = scene_config.cur_camera;
    double cameraAspect = scene_config.cameras[cur_camera]->aspect;

    // some pre-sets
    float blackBarWidth = 0.0f;
    float blackBarHeight = 0.0f;
    if (cameraAspect > windowAspect) {
        blackBarHeight = (static_cast<float>(window_height) - static_cast<float>(swapChainExtent.width) / cameraAspect) / 2.0f;
    }
    else {
        blackBarWidth = (static_cast<float>(window_width) - static_cast<float>(swapChainExtent.height) * cameraAspect) / 2.0f;
    }

    // std::cout << "blackBarWidth: " << blackBarWidth << " blackBarHeight: " << blackBarHeight << std::endl;
    VkViewport viewport {
        .x = blackBarWidth,
        .y = blackBarHeight,
        .width = static_cast<float>(swapChainExtent.width) - 2.0f * blackBarWidth,
        .height = static_cast<float>(swapChainExtent.height) - 2.0f * blackBarHeight,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    
    VkOffset2D scissorOffset {
        .x = static_cast<int32_t>(blackBarWidth),
        .y = static_cast<int32_t>(blackBarHeight)
    };
    VkExtent2D scissorExtent {
        .width = static_cast<uint32_t>(swapChainExtent.width) - 2 * static_cast<uint32_t>(blackBarWidth),
        .height = static_cast<uint32_t>(swapChainExtent.height) - 2 * static_cast<uint32_t>(blackBarHeight)
    };

    VkRect2D scissor {
        .offset = scissorOffset,
        .extent = scissorExtent
    };

    VkDeviceSize offsets[] = {0};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    // draw contents
    auto& cur_frame_material_meshInnerId2ModelMatrices = frame_material_meshInnerId2ModelMatrices[currentFrame];
    int curInstanceIndex = 0;

    VkBuffer vertexBuffers[] = {vertexBuffer};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    
    for (auto& pair : cur_frame_material_meshInnerId2ModelMatrices) {
        MaterialType materialType = pair.first;
        auto& meshInnerId2ModelMatrices = pair.second;
        VkPipeline graphicssPipeline = material2Pipelines[materialType];

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicssPipeline);
        // vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        // vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material2PipelineLayouts[materialType], 0, 1, &descriptorSets[currentFrame], 0, nullptr);

        frameRealDraw(commandBuffer, curInstanceIndex, meshInnerId2ModelMatrices);
    }
    
    // draw cloud
    updateCloudUniformBuffer(currentFrame);
    int cloudIdx = 0;
    for (const auto& [id, cloud] : scene_config.id2clouds) {
        VkBuffer pcloudVertexBuffers[] = { cloudVertexBuffers[cloudIdx] };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, pcloudVertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, cloudIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cloudPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cloudPipelineLayout, 0, 1, &cloudDescriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(cloud->indices.size()), 1, 0, 0, 0);

        cloudIdx++;
    }

    // end drawing, (render pass)
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void SceneViewer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
    
}

void SceneViewer::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // acquire image-index from swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // when there is a window size change, recreate the swap chain
    // if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    //     recreateSwapChain();
    //     return;
    // } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    //     throw std::runtime_error("failed to acquire swap chain image!");
    // }
    
    // Only reset the fence if we are submitting work
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    // submit command buffer to graphics queue
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = { swapChain };
    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
    };

    // make actual present on window
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    // if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    //     framebufferResized = false;
    //     recreateSwapChain();
    // } else if (result != VK_SUCCESS) {
    //     throw std::runtime_error("failed to present swap chain image!");
    // }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void SceneViewer::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void SceneViewer::frameRealDraw(VkCommandBuffer commandBuffer, int& currentInstanceIdx, const std::map<int, std::vector<cglm::Mat44f>>& meshInnerId2ModelMatrices) {
    // std::cout << "MATERIAL: " << materialType << "\n";

    for (auto& p : meshInnerId2ModelMatrices) {
        int meshInnerId = p.first;
        auto& modelMatrices = p.second;
        int meshId = scene_config.innerId2meshId[meshInnerId];
        int vertexCount = scene_config.id2mesh[meshId]->vertex_count;
        int numInstances = modelMatrices.size();
        // vertexIndex is the offset
        int vertexIndex = meshInnerId2Offset[meshInnerId];
        // std::cout << vertexCount << " " << numInstances << " " << vertexIndex << " " << currentInstanceIdx << " | \n";
        if (numInstances == 0) {
            continue;
        }

        // std::cout << meshId << " " << numInstances << " " << vertexIndex << " " << curInstanceIndex << " | \n";
        vkCmdDraw(commandBuffer,
            static_cast<uint32_t>(vertexCount),      /* Vertex Count */
            numInstances,           /* Instance Count */
            vertexIndex,            /* First Vertex, defines lowest value of gl_VertexIndex */
            currentInstanceIdx        /* First Instance Index, defines lowest of gl_InstanceIndex */
        );
        currentInstanceIdx += numInstances;
    }

    // std::cout << std::endl;
    // std::cout << "currentInstanceIdx: " << currentInstanceIdx << std::endl;

}