#include "scene_viewer.hpp"
#include "headless.hpp"

// this is the file we are going to implement headless mode

void SceneViewer::run_headless(std::string& events) {
    is_headless = true;
    std::vector<std::shared_ptr<Event>> evs;
    parseEvents(events, evs);

    // scene file should be loaded before the events
    scene_config.load_scene(scene_file);
    loadCheck();

    // then initHeadlessVulkan
    initHeadlessVulkan();

    copyAllMeshVertexToBuffer();
    startTime = std::chrono::high_resolution_clock::now();

    // headless loop
    double currentRate = 1.0;
    long long prevTimeStamps = 0;
    for (auto ev : evs) {
        if (ev->type == MARK) {
            std::cout << "MARK: " << ev->args << std::endl;
        }
        if (ev->type == PLAY) {
            // std::cout << "PLAY: " << ev->t << " " << ev->rate << std::endl;
            currentRate = ev->rate;
            prevTimeStamps = ev->timestamp;
            // drawHeadlessFrame();  // play actually do not need to draw
        }
        if (ev->type == AVAILABLE) {
            // std::cout << "AVAILABLE: " << ev->timestamp << std::endl;
            long long diffTime = ev->timestamp - prevTimeStamps;
            double diffTimeSec = diffTime / 1000000.0 * currentRate;
            // std::cout << "diffTimeSec: " << diffTimeSec << std::endl;
            setup_frame_instances(diffTimeSec);
            drawHeadlessFrame();
        }
        if (ev->type == SAVE) {
            // std::cout << "SAVE: " << ev->args << std::endl;
            std::string filename = ev->args;
            // get image and save it
            setup_frame_instances(-1);

            // headlessFrameFetch();

            saveImage(ev->args);
        }
    }

    vkDeviceWaitIdle(device);

    // headless cleanup
    headlessCleanup();
    // cleanup();
}


void SceneViewer::headlessFrameFetch() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // acquire image-index from swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    updateUniformBuffer(currentFrame);
    
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
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .swapchainCount = 0,
        .pSwapchains = nullptr,
        .pImageIndices = nullptr,
    };

    // make actual present on window
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void SceneViewer::copyToDstImage() {
    vkResetCommandBuffer(copyCmdBuffers[currentFrame], 0);
    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };
    if (vkBeginCommandBuffer(copyCmdBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // Transition destination image to transfer destination layout
    VkImageMemoryBarrier imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = dstImage,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };

    vkCmdPipelineBarrier(
        copyCmdBuffers[currentFrame],
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &imageMemoryBarrier
    );


    VkImageCopy imageCopyRegion {
        .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .srcOffset = {0, 0, 0},
        .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .dstOffset = {0, 0, 0},
        .extent = {static_cast<uint32_t>(window_width), static_cast<uint32_t>(window_height), 1},
    };


    vkCmdCopyImage(
        copyCmdBuffers[currentFrame],
        colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageCopyRegion
    );

    VkImageMemoryBarrier imageMemoryReadBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = dstImage,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };

    vkCmdPipelineBarrier(
        copyCmdBuffers[currentFrame],
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &imageMemoryReadBarrier
    );

    vkEndCommandBuffer(copyCmdBuffers[currentFrame]);
    
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphores[currentFrame],
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &copyCmdBuffers[currentFrame],
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = 0,
    };
    VkFence copyFence;
    vkCreateFence(device, &fenceInfo, nullptr, &copyFence);
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, copyFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    vkWaitForFences(device, 1, &copyFence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, copyFence, nullptr);

}


void SceneViewer::saveImage(std::string filename) {
    // Get layout of the image (including row pitch)
    VkImageSubresource subResource {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .arrayLayer = 0,
    };
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(device, dstImage, &subResource, &subResourceLayout);

    // Map image memory so we can start copying from it
    char* data;
    vkMapMemory(device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);

    const char* filename_c = filename.c_str();
    std::ofstream file(filename_c, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to open file for writing!");
    }

    // ppm header
    file << "P6\n" << window_width << "\n" << window_height << "\n" << 255 << "\n";
    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
    const bool colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());

    for (int32_t y = 0; y < window_height; y++) {
        unsigned int *row = (unsigned int*)data;
        for (int32_t x = 0; x < window_width; x++) {
            if (colorSwizzle) {
                file.write((char*)row + 2, 1);
                file.write((char*)row + 1, 1);
                file.write((char*)row, 1);
            }
            else {
                file.write((char*)row, 3);
            }
            row++;
        }
        data += subResourceLayout.rowPitch;
    }
    file.close();

    vkUnmapMemory(device, dstImageMemory);
}



void SceneViewer::drawHeadlessFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // making frame unique MVP
    updateUniformBuffer(currentFrame);

    // resetFences need a correct logic
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);

    recordHeadlessCommandBuffer(commandBuffers[currentFrame]);

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinishedSemaphores[currentFrame],
    };

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // then I make copy to dst
    copyToDstImage();
    

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void SceneViewer::recordHeadlessCommandBuffer(VkCommandBuffer commandBuffer) {
    // begin recording command buffer
    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // begin drawing, (render pass)
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{1.0f, 0.5976f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = headlessFramebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = {static_cast<uint32_t>(window_width), static_cast<uint32_t>(window_height)},
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Viewport and scissor
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)window_width,
        .height = (float)window_height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {static_cast<uint32_t>(window_width), static_cast<uint32_t>(window_height)},
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // bind the graphics pipeline    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // bind vertex buffer
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    // draw
    // frameRealDraw(commandBuffer, MaterialType::simple);

    // end drawing
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}


// headless vulkan initialziation
void SceneViewer::initHeadlessVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
    createHeadlessLogicalDevice();

    createHeadlessRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipelines();
    createHeadlessCommandPool();
    // what about frame buffers?
    createHeadlessFramebuffers();

    createVertexBuffer();
    createUniformBuffers();

    createDescriptorPool();
    createDescriptorSets();
    createHeadlessCommandBuffers();
    createSyncObjects();

    createDstImage();
}


void SceneViewer::createDstImage() {
    VkImageCreateInfo imgCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {static_cast<uint32_t>(window_width), static_cast<uint32_t>(window_height), 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkResult result = vkCreateImage(device, &imgCreateInfo, nullptr, &dstImage);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }

    vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    result = vkAllocateMemory(device, &allocInfo, nullptr, &dstImageMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
    }
    vkBindImageMemory(device, dstImage, dstImageMemory, 0);
}


void SceneViewer::headlessCleanup() {
    
    vkFreeMemory(device, dstImageMemory, nullptr);
    vkDestroyImage(device, dstImage, nullptr);

    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyFramebuffer(device, headlessFramebuffer, nullptr);

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroyInstance(instance, nullptr);
}