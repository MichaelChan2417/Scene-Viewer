#include "scene_viewer.hpp"

std::vector<std::vector<Vertex>> frame_vertices_static(MAX_FRAMES_IN_FLIGHT);
std::vector<Vertex> static_vertices;
std::vector<Vertex> indexed_vertices;

bool SceneViewer::leftMouseButtonPressed = false;
bool SceneViewer::rightMouseButtonPressed = false;
double SceneViewer::lastXPos;
double SceneViewer::lastYPos;

void SceneViewer::initVulkan() {
    std::cout << "Init Vulkan..." << std::endl;
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();

    createSwapChain();
    createImageViews();

    // something should also pull ahead
    createRenderPass(swapChainImageFormat, renderPass, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    std::cout << 0 << std::endl;
    createDescriptorSetLayout();
    createCloudDescriptorSetLayout();
    std::cout << 1 << std::endl;
    createGraphicsPipelines();
    std::cout << "Go Cloud " << std::endl;
    createCloudPipeline();
    std::cout << 2 << std::endl;
    createCommandPool();
    std::cout << 3 << std::endl;
    createDepthResources();
    std::cout << 4 << std::endl;
    createFramebuffers();

    // createTextureImage();
    // createTextureImageView();
    createTextureImagesWithViews();
    createCloudImagesWithViews();
    std::cout << 5 << std::endl;
    createTextureSampler();
    createCloudSampler();
    std::cout << 6 << std::endl;

    createVertexBuffer();
    createCloudVertexBuffers();
    std::cout << 7 << std::endl;
    createIndexBuffer();
    createUniformBuffers();
    createCloudUniformBuffers();

    // Light resources should pull ahead!!!
    createLightResources();

    createDescriptorPool();
    createCloudDescriptorPool();
    std::cout << 8 << std::endl;
    createDescriptorSets();
    createCloudDescriptorSets();
    std::cout << 9 << std::endl;
    createCommandBuffers();
    std::cout << 10 << std::endl;
    createSyncObjects();
    std::cout << 11 << std::endl;

}

void SceneViewer::cleanup() {

    cleanupSwapChain();

    // clear shadow depth resources
    cleanShadowResources();

    // clear cloud resources
    cleanCloudResources();

    vkDestroySampler(device, textureSampler2D, nullptr);
    vkDestroySampler(device, textureSamplerCube, nullptr);

    for (auto& imageView : texture2DImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    for (auto& image : texture2DImages) {
        vkDestroyImage(device, image, nullptr);
    }
    for (auto& memory : texture2DImageMemorys) {
        vkFreeMemory(device, memory, nullptr);
    }

    for (auto& imageView : textureCubeImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    for (auto& image : textureCubeImages) {
        vkDestroyImage(device, image, nullptr);
    }
    for (auto& memory : textureCubeImageMemorys) {
        vkFreeMemory(device, memory, nullptr);
    }

    // vkDestroyPipeline(device, graphicsPipeline, nullptr);
    for (auto& [materialType, pipeline] : material2Pipelines) {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
    // vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    for (auto& [materialType, cpipelineLayout] : material2PipelineLayouts) {
        vkDestroyPipelineLayout(device, cpipelineLayout, nullptr);
    }
    vkDestroyRenderPass(device, renderPass, nullptr);

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

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void SceneViewer::loadCheck() {
    // initialize frame instances
    frame_material_meshInnerId2ModelMatrices.resize(MAX_FRAMES_IN_FLIGHT);

    texturePrepare();

    std::cout << "In loadCheck(), we have " << scene_config.id2instance.size() << " instances" << std::endl;

    // check if we have this camera
    if (scene_config.cameras.find(scene_config.cur_camera) == scene_config.cameras.end()) {
        throw std::runtime_error("Camera " + scene_config.cur_camera + " not found in scene file");
    }
    scene_config.cur_camera = camera_name;
}

void SceneViewer::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);      // disable window resize
    // std::cout << "Window width: " << window_width << " " << window_height << std::endl;
    window = glfwCreateWindow(window_width, window_height, "Michael_NICE_Window", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    // glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetMouseButtonCallback(window, mouse_control_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scroll_callback);
}

void SceneViewer::mainLoop() {

    copyAllMeshVertexToBuffer();
    copyCloudVertexToBuffer();

    startTime = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        setup_frame_instances(-1);

        drawFrame();
        // get current time
        // auto currentTime = std::chrono::high_resolution_clock::now();
        // count int milliseconds
        // double dtime = std::chrono::duration<double, std::chrono::milliseconds::period>(currentTime - startTime).count();
        // std::cout << dtime << std::endl;
        // startTime = currentTime;
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

    // i-73, o-79
    if (key_map[73]) {
        // rotate around dir axis
        cglm::Mat44f rot = cglm::rotate(dir, cglm::to_radians(-2.0f));
        cglm::Vec3f new_up = rot * up;
        camera->up = cglm::normalize(new_up);
    }
    if (key_map[79]) {
        // rotate around dir axis
        cglm::Mat44f rot = cglm::rotate(dir, cglm::to_radians(2.0f));
        cglm::Vec3f new_up = rot * up;
        camera->up = cglm::normalize(new_up);
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

    // print camera info
    // std::cout << "Camera Position: " << camera->position[0] << " " << camera->position[1] << " " << camera->position[2] << std::endl;
    // std::cout << "Camera Dir: " << camera->dir[0] << " " << camera->dir[1] << " " << camera->dir[2] << std::endl;
    // std::cout << "Camera Up: " << camera->up[0] << " " << camera->up[1] << " " << camera->up[2] << std::endl;

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
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rightMouseButtonPressed = true;
            glfwGetCursorPos(window, &lastXPos, &lastYPos);
        }
        else if (action == GLFW_RELEASE) {
            rightMouseButtonPressed = false;
        }
    }
}

void SceneViewer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto app = reinterpret_cast<SceneViewer*>(glfwGetWindowUserPointer(window));

    // only debug user & debug can be controlled
    if (app->scene_config.cur_camera != "user" && app->scene_config.cur_camera != "debug") {
        return;
    }

    std::shared_ptr<sconfig::Camera> camera = app->scene_config.cameras[app->scene_config.cur_camera];
    cglm::Vec3f dir = normalize(camera->dir);

    cglm::Vec3f new_pos = camera->position + dir * 0.1f * static_cast<float>(yoffset);
    camera->position = new_pos;
}

void SceneViewer::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    double deltaX = xpos - lastXPos;
    double deltaY = ypos - lastYPos;

    glfwGetCursorPos(window, &lastXPos, &lastYPos);

    if (deltaX == 0 && deltaY == 0) {
        return;
    }

    auto app = reinterpret_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
    // only debug user & debug can be controlled
    if (app->scene_config.cur_camera != "user" && app->scene_config.cur_camera != "debug") {
        return;
    }

    std::shared_ptr<sconfig::Camera> camera = app->scene_config.cameras[app->scene_config.cur_camera];

    // camera moving
    if (leftMouseButtonPressed && rightMouseButtonPressed) {
        // move up and down
        cglm::Vec3f dir_axis = normalize(camera->up);
        cglm::Vec3f new_pos = camera->position + dir_axis * 0.01f * static_cast<float>(deltaY);
        // move left and right
        cglm::Vec3f right_axis = normalize(cglm::cross(camera->dir, camera->up));
        new_pos = new_pos - right_axis * 0.01f * static_cast<float>(deltaX);
        camera->position = new_pos;
    }

    // move will trigger this callback.
    else if (leftMouseButtonPressed) {
        // rotate around up axis
        cglm::Mat44f rot_up = cglm::rotate(camera->up, static_cast<float>(cglm::to_radians(0.1f) * deltaX));
        // need to calculate cross to get rotate2 axis
        cglm::Vec3f rot_axis = cglm::cross(camera->dir, camera->up);
        cglm::Mat44f rot2 = cglm::rotate(rot_axis, static_cast<float>(cglm::to_radians(0.1f) * deltaY));

        cglm::Vec3f new_dir = rot2 * rot_up * camera->dir;
        cglm::Vec3f new_up = rot2 * camera->up;

        //if any value is nan, throw error
        if (std::isnan(new_up[0]) || std::isnan(new_up[1]) || std::isnan(new_up[2]) || std::isnan(new_dir[0]) || std::isnan(new_dir[1]) || std::isnan(new_dir[2])) {
            throw std::runtime_error("NAN value detected in new_up or new_dir");
        }

        camera->up = new_up;
        camera->dir = new_dir;
    }
}

void SceneViewer::setup_frame_instances(double inTime) {
    // start from root, make each dfs, using currentFrame
    frame_material_meshInnerId2ModelMatrices[currentFrame].clear();

    cglm::Mat44f identity_m = cglm::identity(1.0f);

    auto currentTime = std::chrono::high_resolution_clock::now();
    double dtime = std::chrono::duration<double, std::chrono::seconds::period>(currentTime - startTime).count();
    if (inTime != -1) {
        dtime = inTime;
    }
    // for each driver, assign current animation_transform matrix
    if (animationPlay) {
        for (auto& [name, driver] : scene_config.name2driver) {
            // if (!driver->useful) {
            //     std::cout << name << " is not useful, skip" << std::endl;
            //     continue;
            // }
            cglm::Mat44f animation_transform = driver->getCurrentTransform(dtime);

            if (driver->light_driver) {
                int node_id = driver->node;
                std::shared_ptr<sconfig::Node> light_node = scene_config.id2node[node_id];
                int light_id = light_node->light_id;
                std::shared_ptr<sconfig::Light> light = scene_config.id2lights[light_id];
                if (driver->channel == "translation") {
                    cglm::Vec4f npos = animation_transform * cglm::Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
                    light->position = { npos[0] / npos[3], npos[1] / npos[3], npos[2] / npos[3] };
                    // std::cout << "Light Position: " << light->position[0] << " " << light->position[1] << " " << light->position[2] << std::endl;
                }
                else if (driver->channel == "rotation") {
                    light->direction = animation_transform * cglm::Vec3f{ 0.0f, 0.0f, -1.0f };
                    light->up = animation_transform * cglm::Vec3f{ 0.0f, 1.0f, 0.0f };
                }
                continue;
            }

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
                break;
            }
            ++ii;
        }

        // if (!visible) {
        //     // std::cout << "Mesh " << mesh_id << " is not visible" << std::endl;
        //     continue;
        // }

        // based on material & mesh, insert it
        int material_id = scene_config.id2mesh[mesh_id]->material_id;
        // std::cout << "Material Id: " << material_id << std::endl;
        MaterialType materialType = scene_config.id2material[material_id]->matetial_type;
        int inner_id = scene_config.id2mesh[mesh_id]->inner_id;
        frame_material_meshInnerId2ModelMatrices[currentFrame][materialType][inner_id].push_back(curTransform);
        // std::cout << "Material " << materialType << " InnerId " << inner_id << std::endl;
    }

}
