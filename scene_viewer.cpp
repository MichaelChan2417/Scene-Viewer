#include "scene_viewer.hpp"

std::vector<std::vector<Vertex>> frame_vertices_static(MAX_FRAMES_IN_FLIGHT);
std::vector<Vertex> indexed_vertices;

bool SceneViewer::leftMouseButtonPressed = false;
double SceneViewer::lastXPos;
double SceneViewer::lastYPos;

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
    createCommandPool();
    createDepthResources();
    createFramebuffers();

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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);      // disable window resize
    // std::cout << "Window width: " << window_width << " " << window_height << std::endl;
    window = glfwCreateWindow(window_width, window_height, "Michael_NICE_Window", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    // glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    // glfwSetMouseButtonCallback(window, mouse_control_callback);
    // glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, keyCallback);
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


void SceneViewer::mouse_control_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftMouseButtonPressed = true;
            glfwGetCursorPos(window, &lastXPos, &lastYPos);
        } else if (action == GLFW_RELEASE) {
            leftMouseButtonPressed = false;
        }
    }
}

void SceneViewer::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (leftMouseButtonPressed) {
        double deltaX = xpos - lastXPos;
        double deltaY = ypos - lastYPos;

        lastXPos = xpos;
        lastYPos = ypos;

        if (deltaX == 0 || deltaY == 0) {
            return;
        }

        auto app = reinterpret_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
        // only debug user & debug can be controlled
        if (app->scene_config.cur_camera != "user" && app->scene_config.cur_camera != "debug") {
            return;
        }

        // std::shared_ptr<sconfig::Camera> camera = app->scene_config.cameras[app->scene_config.cur_camera];
        // // rotate around up axis
        // cglm::Mat44f rot_up = cglm::rotate(camera->up, static_cast<float>(cglm::to_radians(5.0f) * deltaX));
        // // need to calculate cross to get rotate2 axis
        // cglm::Vec3f rot_axis = cglm::cross(camera->dir, camera->up);
        // cglm::Mat44f rot2 = cglm::rotate(rot_axis, static_cast<float>(cglm::to_radians(5.0f) * deltaY));

        // cglm::Vec3f new_dir = rot_up * camera->dir;
        // cglm::Vec3f new_up = rot2 * camera->up;

        // std::cout << "New Up: " << new_up[0] << " " << new_up[1] << " " << new_up[2] << std::endl;
        // std::cout << "New Dir: " << new_dir[0] << " " << new_dir[1] << " " << new_dir[2] << std::endl;

        // //if any value is nan, throw error
        // if (std::isnan(new_up[0]) || std::isnan(new_up[1]) || std::isnan(new_up[2]) || std::isnan(new_dir[0]) || std::isnan(new_dir[1]) || std::isnan(new_dir[2])) {
        //     throw std::runtime_error("NAN value detected in new_up or new_dir");
        // }

        // camera->up = new_up;
        // camera->dir = new_dir;

        std::cout << "DeltaX: " << deltaX << " DeltaY: " << deltaY << std::endl;
    }
}


// based on current scene/camera, assign the current frame with correct vertex data
void SceneViewer::assignCurrentFrame() {
    std::shared_ptr<sconfig::Camera> camera = scene_config.cameras[scene_config.cur_camera];
    frame_vertices_static[currentFrame].clear();
    // std::cout << "Current Camera: " << scene_config.cur_camera << std::endl;
    for (auto& [id, mesh] : scene_config.id2mesh) {
        // cglm::Vec3f camera_pos = { camera->position[0], camera->position[1], camera->position[2] };
        // cglm::Vec3f dir = cglm::Vec3f{ 0.0f, 0.0f, 0.0f } - camera_pos;
        // auto currentTime = std::chrono::high_resolution_clock::now();
        // float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        // cglm::Mat44f rot_z = cglm::rotate({ 0.0f, 0.0f, 1.0f }, time * cglm::to_radians(30.0f));
        // cglm::Vec3f view_point = camera_pos + rot_z * dir;

        // cglm::Mat44f view_matrix = cglm::lookAt(camera_pos, view_point, cglm::Vec3f{ 0.0f, 0.0f, 1.0f });
        // cglm::Mat44f proj_matrix = cglm::perspective(camera->vfov, camera->aspect, camera->near, camera->far);

        // cglm::Vec4f pos = { mesh->center[0], mesh->center[1], mesh->center[2], 1.0f };
        // cglm::Vec4f proj_pos = proj_matrix * view_matrix * pos;
        // proj_pos /= proj_pos[3];

        for (int i = 0; i < mesh->vertex_count; i++) {
            Vertex vertex{};
            vertex.pos = { mesh->positions[i * 3], mesh->positions[i * 3 + 1], mesh->positions[i * 3 + 2] };
            vertex.color = { mesh->colors[i * 3], mesh->colors[i * 3 + 1], mesh->colors[i * 3 + 2] };
            cglm::Vec3f norm = { mesh->normals[i * 3], mesh->normals[i * 3 + 1], mesh->normals[i * 3 + 2] };
            if (dot(norm, camera->dir) > 0) {
                continue;
            }
            // std::cout << "Color is " << mesh->colors[i * 3] << " " << mesh->colors[i * 3 + 1] << " " << mesh->colors[i * 3 + 2] << std::endl;
            frame_vertices_static[currentFrame].push_back(vertex);
        }
    }

    std::cout << "Static Vertices: " << frame_vertices_static[currentFrame].size() << std::endl;
    // std::cout << "Total Vertex Count: " << scene_config.get_total_vertex_count() << std::endl;
    copyVertexToBuffer();
}

void SceneViewer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // w:87 a:65 s:83 d:68 up:265 down:264 left:263 right:262 u:85 n:78
    std::unordered_map<int, bool> key_map;
    if (action == GLFW_PRESS) {
        key_map[key] = true;
    } else if (action == GLFW_RELEASE) {
        key_map[key] = false;
    }

    auto app = reinterpret_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
    if (app->scene_config.cur_camera != "user" && app->scene_config.cur_camera != "debug") {
        return;
    }

    std::shared_ptr<sconfig::Camera> camera = app->scene_config.cameras[app->scene_config.cur_camera];
    cglm::Vec3f dir = normalize(camera->dir);
    cglm::Vec3f up = normalize(camera->up);
    cglm::Vec3f right = normalize(cglm::cross(dir, up));

    if (key_map[263]) {
        cglm::Mat44f rot = cglm::rotate(up, cglm::to_radians(2.0f));
        cglm::Vec3f new_dir = rot * dir;
        camera->dir = new_dir;
    }

    if (key_map[262]) {
        cglm::Mat44f rot = cglm::rotate(up, cglm::to_radians(-2.0f));
        cglm::Vec3f new_dir = rot * dir;
        camera->dir = new_dir;
    }

    if (key_map[265]) {
        cglm::Mat44f rot = cglm::rotate(right, cglm::to_radians(2.0f));
        cglm::Vec3f new_dir = rot * dir;
        camera->dir = new_dir;
    }

    if (key_map[264]) {
        cglm::Mat44f rot = cglm::rotate(right, cglm::to_radians(-2.0f));
        cglm::Vec3f new_dir = rot * dir;
        camera->dir = new_dir;
    }

    // now control camera movement
    if (key_map[87]) {
        cglm::Vec3f new_pos = camera->position + dir * 0.1f;
        camera->position = new_pos;
    }

    if (key_map[83]) {
        cglm::Vec3f new_pos = camera->position - dir * 0.1f;
        camera->position = new_pos;
    }

    if (key_map[65]) {
        cglm::Vec3f new_pos = camera->position - right * 0.1f;
        camera->position = new_pos;
    }

    if (key_map[68]) {
        cglm::Vec3f new_pos = camera->position + right * 0.1f;
        camera->position = new_pos;
    }

    if (key_map[85]) {
        // make camera up
        cglm::Vec3f new_pos = camera->position + up * 0.1f;
        camera->position = new_pos;
    }

    if (key_map[78]) {
        // make camera down
        cglm::Vec3f new_pos = camera->position - up * 0.1f;
        camera->position = new_pos;
    }

    
}