#include "../scene_viewer.hpp"

void SceneViewer::pickPhysicalDevice() {
    if (device_name.has_value()) {
        pickPhysicalDeviceWithSpecifiedName();
        return;
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isPhysicalDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    printPhysicalDeviceInfo();
}


QueueFamilyIndices SceneViewer::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int idx = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = idx;
        }
        
        // we also want a queue-family able to present to the window surface
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = idx;
        }

        if (indices.isComplete()) {
            break;
        }
        ++idx;
    }

    return indices;
}


bool SceneViewer::isPhysicalDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    // check for extension support
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // but this is not enough, we also need to check for swap chain support
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.graphicsFamily.has_value() && extensionsSupported && swapChainAdequate;
}


void SceneViewer::pickPhysicalDeviceWithSpecifiedName() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if (deviceProperties.deviceName == device_name.value()) {
            physicalDevice = device;
            break;
        }
    }

    // TODO: what if selected one is not suitable? here we just reject it
    if (!isPhysicalDeviceSuitable(physicalDevice)) {
        throw std::runtime_error("Selected Device is not suitable!");
    }
    
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    printPhysicalDeviceInfo();
}


void SceneViewer::list_physical_devices() {
    // we still need a instance to list all physical devices :(
    createInstance();

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        std::cout << "No physical device found!" << std::endl;
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    VkPhysicalDeviceProperties deviceProperties;
    std::cout << "Device Lists:\n";
    for (const auto& device : devices) {
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << deviceProperties.deviceName << std::endl;
    }
}

void SceneViewer::printPhysicalDeviceInfo() {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout << "[Info] Selected physical device: " << deviceProperties.deviceName << std::endl;
}