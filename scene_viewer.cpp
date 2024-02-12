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


void SceneViewer::loadCheck() {
    // check if we have this camera
    if (scene_config.cameras.find(scene_config.cur_camera) == scene_config.cameras.end()) {
        throw std::runtime_error("Camera " + scene_config.cur_camera + " not found in scene file");
    }
    scene_config.cur_camera = camera_name;
}

// based on current scene/camera, assign the current frame with correct vertex data
void SceneViewer::assignCurrentFrame() {
    std::shared_ptr<sconfig::Camera> camera = scene_config.cameras[scene_config.cur_camera];
    frame_vertices_static[currentFrame].clear();
    std::cout << "Current Camera: " << scene_config.cur_camera << " Culling " << culling << std::endl;
    std::vector<std::shared_ptr<sconfig::Plane>> bound_spheres = camera->bounds;
    std::cout << camera->position << " With Direction: " << camera->dir << std::endl;

    // I should use the scene
    std::shared_ptr<sconfig::Scene> scene = scene_config.scene;
    for (auto& child_node : scene->children) {
        std::shared_ptr<sconfig::Node> node = scene_config.id2node[child_node];

        // case - 1: no culling, draw everything
        if (culling == "none") {
            for (int i = 0; i < node->vertex_count; i++) {
                cglm::Vec3f norm = normalize(node->normals[i]);
                Vertex vertex{};
                vertex.pos = node->positions[i];
                vertex.color = node->colors[i];
                frame_vertices_static[currentFrame].push_back(vertex);
            }
            continue;
        }

        // case - 2: so we only have frustum culling
        std::vector<std::shared_ptr<sconfig::Bound_Sphere>> bs_instances = node->bound_spheres;
        for (auto& bs : bs_instances) {
            cglm::Vec3f center = bs->center;
            float radius = bs->radius;

            // std::cout << "Center: " << center << " Radius: " << radius << std::endl;
            bool in_view = true;
            // test each boundary
            int i = 0;
            for (auto& plane : bound_spheres) {
                cglm::Vec3f normal = plane->normal;
                cglm::Vec3f on_plane_point = plane->normal * plane->d;
                float d2 = cglm::dot(normal, center - on_plane_point);
                if (d2 + radius <= 0) {
                    in_view = false;
                    break;
                }
                ++i;
            }

            if (!in_view) {
                continue;
            }

            // then this instance is in view
            for (int i = bs->startIdx; i < bs->endIdx; i++) {
                cglm::Vec3f norm = normalize(node->normals[i]);
                Vertex vertex{};
                vertex.pos = node->positions[i];
                vertex.color = node->colors[i];
                frame_vertices_static[currentFrame].push_back(vertex);
            }
        }

    }

    std::cout << "Static Vertices: " << frame_vertices_static[currentFrame].size() << std::endl;

    if (frame_vertices_static[currentFrame].size() == 0) {
        std::cout << "No vertex in current frame" << std::endl;
        return;
    }
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
        camera->dir = cglm::normalize(new_dir);
    }

    if (key_map[262]) {
        cglm::Mat44f rot = cglm::rotate(up, cglm::to_radians(-2.0f));
        cglm::Vec3f new_dir = rot * dir;
        camera->dir = cglm::normalize(new_dir);
    }

    if (key_map[265]) {
        cglm::Mat44f rot = cglm::rotate(right, cglm::to_radians(2.0f));
        cglm::Vec3f new_dir = rot * dir;
        camera->dir = cglm::normalize(new_dir);
        camera->up = cglm::normalize(rot * up);
    }

    if (key_map[264]) {
        cglm::Mat44f rot = cglm::rotate(right, cglm::to_radians(-2.0f));
        cglm::Vec3f new_dir = rot * dir;
        camera->dir = cglm::normalize(new_dir);
        camera->up = cglm::normalize(rot * up);
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

    camera->update_planes();
}