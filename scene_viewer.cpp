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
    
    
    startTime = std::chrono::high_resolution_clock::now();
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

void SceneViewer::useless_prepare_vertices() {
    std::shared_ptr<sconfig::Camera> camera = scene_config.cameras[scene_config.cur_camera];

    for (size_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++) {
        frame_vertices_static[idx].clear();
        for (auto& [id, mesh] : scene_config.id2mesh) {
            // check mesh's bounding sphere
            for (int i = 0; i < mesh->vertex_count; i++) {
                Vertex vertex{};
                vertex.pos = { mesh->positions[i * 3], mesh->positions[i * 3 + 1], mesh->positions[i * 3 + 2] };
                vertex.color = { mesh->colors[i * 3], mesh->colors[i * 3 + 1], mesh->colors[i * 3 + 2] };
                frame_vertices_static[idx].push_back(vertex);
            }
        }
    }
    
}


// based on current scene/camera, assign the current frame with correct vertex data
void SceneViewer::assignCurrentFrame() {
    std::shared_ptr<sconfig::Camera> camera = scene_config.cameras[scene_config.cur_camera];
    frame_vertices_static[currentFrame].clear();
    std::cout << "Current Camera: " << scene_config.cur_camera << std::endl;
    for (auto& [id, mesh] : scene_config.id2mesh) {
        cglm::Vec3f camera_pos = { camera->position[0], camera->position[1], camera->position[2] };
        cglm::Vec3f dir = cglm::Vec3f{ 0.0f, 0.0f, 0.0f } - camera_pos;
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        cglm::Mat44f rot_z = cglm::rotate({ 0.0f, 0.0f, 1.0f }, time * cglm::to_radians(30.0f));
        cglm::Vec3f view_point = camera_pos + rot_z * dir;

        cglm::Mat44f view_matrix = cglm::lookAt(camera_pos, view_point, cglm::Vec3f{ 0.0f, 0.0f, 1.0f });
        cglm::Mat44f proj_matrix = cglm::perspective(camera->vfov, camera->aspect, camera->near, camera->far);

        cglm::Vec4f pos = { mesh->center[0], mesh->center[1], mesh->center[2], 1.0f };
        cglm::Vec4f proj_pos = proj_matrix * view_matrix * pos;
        proj_pos /= proj_pos[3];

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