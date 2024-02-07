#include "scene_viewer.hpp"

std::vector<std::vector<Vertex>> frame_vertices_static(MAX_FRAMES_IN_FLIGHT);
std::vector<Vertex> indexed_vertices;

void SceneViewer::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandPool();

    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();

    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void SceneViewer::cleanup() {

    cleanupSwapChain();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

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

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void SceneViewer::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);      // disable window resize
    window = glfwCreateWindow(window_width, window_height, "Michael_NICE_Window", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void SceneViewer::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        assignCurrentFrame();

        drawFrame();
        // std::cout << window_width << " " << window_height << std::endl;
    }
    vkDeviceWaitIdle(device);
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


// based on current scene/camera, assign the current frame with correct vertex data
void SceneViewer::assignCurrentFrame() {
    std::shared_ptr<sconfig::Camera> camera = scene_config.cameras[scene_config.cur_camera];
    frame_vertices_static[currentFrame].clear();
    // std::cout << "Current Camera: " << scene_config.cur_camera << std::endl;
    for (auto& [id, mesh] : scene_config.id2mesh) {
        for (int i = 0; i < mesh->vertex_count; i++) {
            Vertex vertex{};
            vertex.pos = { mesh->positions[i * 3], mesh->positions[i * 3 + 1], mesh->positions[i * 3 + 2] };
            vertex.color = { mesh->colors[i * 3], mesh->colors[i * 3 + 1], mesh->colors[i * 3 + 2] };
            // std::cout << "Color is " << mesh->colors[i * 3] << " " << mesh->colors[i * 3 + 1] << " " << mesh->colors[i * 3 + 2] << std::endl;
            frame_vertices_static[currentFrame].push_back(vertex);
        }
    }

    // std::cout << "Static Vertices: " << static_vertices.size() << std::endl;
    // std::cout << "Total Vertex Count: " << scene_config.get_total_vertex_count() << std::endl;
    copyVertexToBuffer();
}

void SceneViewer::useless_prepare_vertices() {
    std::shared_ptr<sconfig::Camera> camera = scene_config.cameras[scene_config.cur_camera];

    for (size_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++) {
        frame_vertices_static[idx].clear();
        for (auto& [id, mesh] : scene_config.id2mesh) {
            for (int i = 0; i < mesh->vertex_count; i++) {
                Vertex vertex{};
                vertex.pos = { mesh->positions[i * 3], mesh->positions[i * 3 + 1], mesh->positions[i * 3 + 2] };
                vertex.color = { mesh->colors[i * 3], mesh->colors[i * 3 + 1], mesh->colors[i * 3 + 2] };
                frame_vertices_static[idx].push_back(vertex);
            }
        }
    }
    
}