#include "scene_viewer.hpp"

std::vector<std::vector<Vertex>> frame_vertices_static(MAX_FRAMES_IN_FLIGHT);
std::vector<Vertex> static_vertices;
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
    // easyCheckSetup();

    copyAllMeshVertexToBuffer();
    
    startTime = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        setup_frame_instances();

        drawFrame();
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

void SceneViewer::loadCheck() {
    // initialize frame instances
    // initialize frame_uniform_buffers
    frame_instances.resize(MAX_FRAMES_IN_FLIGHT);
    frame_uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // each frame basis is on each mesh, then insert instance
        frame_instances[i].resize(scene_config.id2mesh.size());
    }

    std::cout << "In loadCheck(), we have " << scene_config.id2instance.size() << " instances" << std::endl;

    // check if we have this camera
    if (scene_config.cameras.find(scene_config.cur_camera) == scene_config.cameras.end()) {
        throw std::runtime_error("Camera " + scene_config.cur_camera + " not found in scene file");
    }
    scene_config.cur_camera = camera_name;
}

void SceneViewer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // w:87 a:65 s:83 d:68 up:265 down:264 left:263 right:262 u:85 n:78 space:32
    std::unordered_map<int, bool> key_map;
    if (action == GLFW_PRESS) {
        key_map[key] = true;
    }
    else if (action == GLFW_RELEASE) {
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

    if (key_map[32]) {
        // space, control animation
        app->animationPlay = !app->animationPlay;
    }

    camera->update_planes();
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

void SceneViewer::setup_frame_instances() {
    // start from root, make each dfs, using currentFrame
    for (int i = 0; i < scene_config.id2mesh.size(); i++) {
        frame_instances[currentFrame][i].clear();
    }

    cglm::Mat44f identity_m = cglm::identity(1.0f);

    auto currentTime = std::chrono::high_resolution_clock::now();
    double dtime = std::chrono::duration<double, std::chrono::seconds::period>(currentTime - startTime).count();   
    // for each driver, assign current animation_transform matrix
    if (animationPlay) {
        for (auto& [name, driver] : scene_config.name2driver) {
            if (!driver->useful) {
                std::cout << name << " is not useful, skip" << std::endl;
                continue;
            }
            cglm::Mat44f animation_transform = driver->getCurrentTransform(dtime);
            if (driver->channel == "translation")
                scene_config.id2node[driver->node]->translation = animation_transform;
            else if (driver->channel == "rotation")
                scene_config.id2node[driver->node]->rotation = animation_transform;
            else if (driver->channel == "scale")
                scene_config.id2node[driver->node]->scale = scene_config.id2node[driver->node]->translation = animation_transform;
        }
    }
    
    for (auto node_id : scene_config.scene->children) {
        dfs_instance(node_id, currentFrame, identity_m);
    }

}

void SceneViewer::dfs_instance(int node_id, int currentFrame, cglm::Mat44f parent_transform) {
    std::shared_ptr<sconfig::Node> node = scene_config.id2node[node_id];
    cglm::Mat44f curTransform;
    curTransform = parent_transform * node->translation * node->rotation * node->scale;

    // dfs on children
    for (auto child_id : node->children) {
        dfs_instance(child_id, currentFrame, curTransform);
    }

    // then for all meshes
    for (auto mesh_id : node->mesh) {
        // before push, we need test if we can see it
        std::shared_ptr<sconfig::Bound_Sphere> bound_sphere = scene_config.id2mesh[mesh_id]->bound_sphere;
        // get new center and use scale to simulate new radius
        cglm::Vec4f sub_new_center = curTransform * cglm::Vec4f(bound_sphere->center, 1.0f);
        cglm::Vec3f new_center = { sub_new_center[0] / sub_new_center[3], sub_new_center[1] / sub_new_center[3], sub_new_center[2] / sub_new_center[3] };
        float scale_x = abs(curTransform(0, 0)), scale_y = abs(curTransform(1, 1)), scale_z = abs(curTransform(2, 2));
        float new_radius = bound_sphere->radius * std::max(scale_x, std::max(scale_y, scale_z));
        
        // check with boundaries
        std::vector<std::shared_ptr<sconfig::Plane>> planes = scene_config.cameras[scene_config.cur_camera]->bounds;
        bool visible = true;
        int ii = 0;
        for (auto& plane : planes) {
            float distance = cglm::dot(plane->normal, new_center) - plane->d;
            if (distance + new_radius < 0.0f) {
                visible = false;
                // std::cout << node->name << "\'s " << scene_config.id2mesh[mesh_id]->name << " is not visible on " << ii << std::endl;
                // std::cout << "Old Center: " << bound_sphere->center << " Old Radius: " << bound_sphere->radius << " Scale: " << scale_x << " " << scale_y << " " << scale_z << std::endl;
                // std::cout << "New Center: " << new_center << " New Radius: " << new_radius << std::endl;

                // std::cout << "Camera pos: " << scene_config.cameras[scene_config.cur_camera]->position;
                // std::cout << " Camera dir: " << scene_config.cameras[scene_config.cur_camera]->dir;
                // std::cout << " Camera up: " << scene_config.cameras[scene_config.cur_camera]->up << std::endl;
                // std::cout << "Plane Normal: " << plane->normal << " Plane D: " << plane->d << std::endl;
                break;
            }
            ++ii;
        }

        if (!visible) {
            continue;
        }

        frame_instances[currentFrame][scene_config.id2mesh[mesh_id]->inner_id].push_back(curTransform);
    }
}

void SceneViewer::easyCheckSetup() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frame_vertices_static[i].clear();
        Vertex vertex{};
        vertex.pos = { 0.0f, 0.0f, 0.0f };
        vertex.normal = { 0.0f, 0.0f, 1.0f };
        vertex.color = { 1.0f, 0.0f, 0.0f };
        frame_vertices_static[i].push_back(vertex);

        // next point
        vertex.pos = { 1.0f, 0.0f, 0.0f };
        vertex.normal = { 0.0f, 0.0f, 1.0f };
        vertex.color = { 0.0f, 1.0f, 0.0f };
        frame_vertices_static[i].push_back(vertex);

        // next point
        vertex.pos = { 0.0f, 1.0f, 0.0f };
        vertex.normal = { 0.0f, 0.0f, 1.0f };
        vertex.color = { 0.0f, 0.0f, 1.0f };
        frame_vertices_static[i].push_back(vertex);

        
        // next point
        vertex.pos = { 0.0f, 1.0f, 0.0f };
        vertex.normal = { 0.0f, 0.0f, 1.0f };
        vertex.color = { 0.0f, 0.0f, 1.0f };
        frame_vertices_static[i].push_back(vertex);

        // next point
        vertex.pos = { 1.0f, 0.0f, 0.0f };
        vertex.normal = { 0.0f, 0.0f, 1.0f };
        vertex.color = { 0.0f, 1.0f, 0.0f };
        frame_vertices_static[i].push_back(vertex);

        // next point
        vertex.pos = { 1.0f, 1.0f, 0.0f };
        vertex.normal = { 0.0f, 0.0f, 1.0f };
        vertex.color = { 1.0f, 1.0f, 1.0f };
        frame_vertices_static[i].push_back(vertex);
    }
}

