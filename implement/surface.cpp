#include "../scene_viewer.hpp"

void SceneViewer::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void SceneViewer::createHeadlessSurface() {
    VkHeadlessSurfaceCreateInfoEXT createInfo{
        .sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT,
    };

    if (vkCreateHeadlessSurfaceEXT(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create headless surface!");
    }
}