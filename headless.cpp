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

    // headless loop

    // headless cleanup
    headlessCleanup();
}


// headless vulkan initialziation
void SceneViewer::initHeadlessVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
    createHeadlessLogicalDevice();

    createHeadlessRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createHeadlessCommandPool();
    // what about frame buffers?
    createHeadlessFramebuffers();

    createVertexBuffer();
    createUniformBuffers();

    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void SceneViewer::headlessCleanup() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    vkDestroyFramebuffer(device, headlessFramebuffer, nullptr);

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