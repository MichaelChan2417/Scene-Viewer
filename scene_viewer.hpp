#pragma once

#define GLFW_DLL
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <unordered_set>
#include <cstring>
#include <optional>
#include <cstdint> 
#include <limits> 
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <map>

#include "scene_config.hpp"

const int MAX_INSTANCE = 32;
const int MAX_LIGHT = 8;

struct CloudVertex {
    cglm::Vec3f pos;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{
            .binding = 0,
            .stride = sizeof(CloudVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(CloudVertex, pos);
        return attributeDescriptions;
    }

};

struct Vertex {
    cglm::Vec3f pos;
    cglm::Vec3f normal;
    cglm::Vec3f color;
    cglm::Vec2f texCoord;
    cglm::Vec3f mappingIdxs; // if last element is 0, then it's a 2D texture, otherwise it's a cube texture
    cglm::Vec2f normalMappingIdxs;
    cglm::Vec3f pbrs;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getSimpleAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription> getEnvAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normalMappingIdxs);

        return attributeDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription> getLambertianAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, mappingIdxs);
        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, normalMappingIdxs);

        return attributeDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription> getPbrAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(6);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, mappingIdxs);
        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, normalMappingIdxs);
        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[5].offset = offsetof(Vertex, pbrs);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    cglm::Vec3f cameraPos;
    alignas(16) cglm::Mat44f model;
    cglm::Mat44f view;
    cglm::Mat44f proj;
    cglm::Mat44f instanceModels[MAX_INSTANCE];
};

struct LightUniformBufferObject {
    alignas(16)cglm::Vec4f lightPos[MAX_LIGHT];
    alignas(16)cglm::Vec4f lightDir[MAX_LIGHT];
    alignas(16)cglm::Vec4f lightColor[MAX_LIGHT];

    alignas(16)cglm::Mat44f lightViewMatrix[MAX_LIGHT];
    alignas(16)cglm::Mat44f lightProjMatrix[MAX_LIGHT];

    alignas(16)cglm::Vec4f metadata1[MAX_LIGHT];
    alignas(16)cglm::Vec4f metadata2[MAX_LIGHT];
};

struct CloudUniformBufferObject {
    alignas(16) cglm::Vec4f lightPos;
    alignas(16) cglm::Vec4f lightColor;
    alignas(16) cglm::Vec4f metadata;
};

extern std::vector<Vertex> static_vertices;
extern std::vector<std::vector<Vertex>> frame_vertices_static;

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

enum ASSIGN_RESULT {
    SUCCESS = 0,
    FAILURE = 1
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

// -------------------------------------------------------------------------|
//                              SceneViewer class                           |
// -------------------------------------------------------------------------|
class SceneViewer {
public:
    int window_width = 800, window_height = 600;        // default value of 800 x 600
    sconfig::SceneConfig scene_config;
    std::string scene_file = "";
    std::string camera_name = "debug";
    std::string culling = "none";
    std::string events;
    bool is_headless = false;
    std::optional < std::string > device_name = std::nullopt;

    void run() {
        scene_config.load_scene(scene_file);
        loadCheck();
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

    void run_headless(std::string& events);

    void list_physical_devices();    // list all physical devices

// private:
    // internals
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;                    // logical device       
    VkQueue graphicsQueue;              // graphics queue with logical device
    VkQueue presentQueue;               // present queue with logical device
    VkSurfaceKHR surface;

    // headless 
    uint32_t queueFamilyIndex;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;   // images in the swap chain
    VkFormat swapChainImageFormat;          // format of the swap chain image
    VkExtent2D swapChainExtent;             // extent of the swap chain image
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;        
    VkPipeline graphicsPipeline;
    std::map<MaterialType, VkPipelineLayout> material2PipelineLayouts;
    std::map<MaterialType, VkPipeline> material2Pipelines;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> copyCmdBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkImage depthImage;                 // depth test over image
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::vector<VkImage> texture2DImages;
    std::vector<VkImage> textureCubeImages;
    std::vector<VkDeviceMemory> texture2DImageMemorys;
    std::vector<VkDeviceMemory> textureCubeImageMemorys;
    std::vector<VkImageView> texture2DImageViews;
    std::vector<VkImageView> textureCubeImageViews;
    VkSampler textureSampler2D;
    VkSampler textureSamplerCube;

    // VkBuffer stagingBuffer;
    // VkDeviceMemory stagingBufferMemory; 

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    // light corresponding
    std::vector<VkImage> shadowMapImages;
    std::vector<VkDeviceMemory> shadowMapImageMemorys;
    std::vector<VkImageView> shadowMapImageViews;
    VkSampler shadowMapSampler;
    std::vector<VkImage> shadowMapCubeImages;
    std::vector<VkDeviceMemory> shadowMapCubeImageMemorys;
    std::vector<VkImageView> shadowMapCubeImageViews;
    std::vector<std::vector<VkImageView>> shadowMapCubeFacesImageViews;
    VkSampler shadowMapCubeSampler;

    VkRenderPass shadowRenderPass;
    std::vector<VkFramebuffer> shadowMapFramebuffers;
    std::vector<std::vector<VkFramebuffer>> shadowMapCubeFramebuffers;
    
    VkPipelineLayout shadowPipelineLayout;
    VkPipeline shadowGraphicsPipeline;
    VkDescriptorSetLayout shadowDescriptorSetLayout;
    std::vector<VkBuffer> shadowUniformBuffers;
    std::vector<VkDeviceMemory> shadowUniformBuffersMemory;
    std::vector<void*> shadowUniformBuffersMapped;
    VkDescriptorPool shadowDescriptorPool;
    std::vector<VkDescriptorSet> shadowDescriptorSets;

    std::vector<VkImage> shadowDepthImages;                 // depth test over image
    std::vector<VkDeviceMemory> shadowDepthImageMemorys;
    std::vector<VkImageView> shadowDepthImageViews;
    std::vector<VkImage> shadowCubeDepthImages;                 // depth test over image
    std::vector<VkDeviceMemory> shadowCubeDepthImageMemorys;
    std::vector<VkImageView> shadowCubeDepthImageViews;

    // clouds
    std::vector<VkBuffer> cloudVertexBuffers;
    std::vector<VkDeviceMemory> cloudVertexBufferMemorys;
    VkBuffer cloudIndexBuffer;
    VkDeviceMemory cloudIndexBufferMemory;
    VkPipelineLayout cloudPipelineLayout;
    VkPipeline cloudPipeline;
    VkDescriptorSetLayout cloudDescriptorSetLayout;
    VkDescriptorPool cloudDescriptorPool;
    std::vector<VkDescriptorSet> cloudDescriptorSets;
    std::vector<VkImage> cloudImages;
    std::vector<VkDeviceMemory> cloudImageMemorys;
    std::vector<VkImageView> cloudImageViews;
    VkSampler cloudSampler;
    VkImage cloudNoiseImage;
    VkDeviceMemory cloudNoiseImageMemory;
    VkImageView cloudNoiseImageView;
    VkSampler cloudNoiseSampler;
    std::vector<VkBuffer> cloudUniformBuffers;
    std::vector<VkDeviceMemory> cloudUniformBuffersMemory;
    std::vector<void*> cloudUniformBuffersMapped;

    // headless specs
    VkFramebuffer headlessFramebuffer;
    VkImage dstImage;
    VkDeviceMemory dstImageMemory;
    VkMemoryRequirements memRequirements;

    std::chrono::high_resolution_clock::time_point startTime;

    static bool leftMouseButtonPressed;
    static bool rightMouseButtonPressed;
    static double lastXPos, lastYPos;
    bool animationPlay = false;

    std::unordered_map<int, int> meshInnerId2Offset;
    std::vector<std::map<MaterialType, std::map<int, std::vector<cglm::Mat44f>>>> frame_material_meshInnerId2ModelMatrices; // this is used for drawing

    // interfaces
    void initWindow();
    void initVulkan();
    void initHeadlessVulkan();
    void mainLoop();
    void cleanup();
    void headlessCleanup();
    void loadCheck();

    static void mouse_control_callback(GLFWwindow* window, int button, int action, int mods);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    // =================== inner functions ===================
    void setup_frame_instances(double inTime);
    void dfs_instance(int node_id, int currentFrame, cglm::Mat44f parent_transform);
    void saveImage(std::string filename);
    void createDstImage();
    void copyToDstImage();
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

    void createInstance();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    // depth management
    void createDepthResources();
    void createHeadlessDepthResources();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);

    // cloud management
    void createCloudVertexBuffers();
    void copyCloudVertexToBuffer();
    void createCloudPipeline();
    void createCloudDescriptorSetLayout();
    void createCloudDescriptorPool();
    void createCloudDescriptorSets();
    void createCloudImagesWithViews();
    void createCloudSampler();
    void createCloudImages(const std::string& filename, int idx);
    void createCloudNoiseImageWithView();
    void createCloudUniformBuffers();
    void updateCloudUniformBuffer(uint32_t currentImage);
    void cleanCloudResources();

    // light source management
    void lightSetup();
    void createLightResources();
    void createShadowDescriptorSetLayout();
    void createShadowGraphicsPipeline();
    void createLightFrameBuffers();
    void createLightImagewithViews();
    void createShadowMapSampler();
    void createLightUniformBuffers();
    void createLightDescriptorPool();
    void createLightDescriptorSets();
    void updateLightUniformBuffer(uint32_t currentImage, int idx, int light_id);
    void updateWholeLightUniformBuffer(uint32_t currentImage, LightUniformBufferObject& lubo);
    void updateCurLightUBOIndex(uint32_t currentFrame, int idx, LightUniformBufferObject& lubo);
    void singleShadowRenderPass(VkCommandBuffer commandBuffer, int spot_idx, int sphere_idx, int light_id);
    void singleCubeShadowRenderPass(VkCommandBuffer commandBuffer, int spot_idx, int sphere_idx, int light_id);
    void cleanShadowResources();

    // texture
    void createTextureImage();
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount);
    void createTextureImageView();
    void createTextureSampler();
    void texturePrepare();
    void createTextureImagesWithViews();
    void createTextureImage2D(const std::string& file_name, int idx);
    void createTextureImageCube(const std::string& file_name, int idx);

    // Uniform buffer
    void createDescriptorSetLayout();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);
    void createDescriptorPool();
    void createDescriptorSets();

    // vertexbuffer
    void createVertexBuffer();
    void createIndexBuffer();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyVertexToBuffer();
    void copyAllMeshVertexToBuffer();

    // framebuffer
    void createFramebuffers();
    void createHeadlessFramebuffers();
    void createColorResources();
    void createCommandPool();
    void createHeadlessCommandPool();
    void createCommandBuffers();
    void createHeadlessCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void recordHeadlessCommandBuffer(VkCommandBuffer commandBuffer);
    void drawFrame();
    void drawHeadlessFrame();
    void createSyncObjects();
    void frameRealDraw(VkCommandBuffer commandBuffer, int& currentInstanceIdx, const std::map<int, std::vector<cglm::Mat44f>>& meshInnerId2ModelMatrices);
    void headlessFrameFetch();

    // graphics pipeline
    void createGraphicsPipeline(MaterialType mt);
    void createGraphicsPipelines();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createRenderPass(VkFormat format, VkRenderPass& pRenderpass, VkImageLayout colorFinalLayout);
    void createHeadlessRenderPass();

    // image views
    void createImageViews();
    VkImageView createImageView2D(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkImageView createImageViewCube(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createImage(uint32_t width, uint32_t height, VkImageCreateFlags flags, VkImageType imageType, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, int layers);

    // swap chain
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();

    // window surface
    void createSurface();
    void createHeadlessSurface();

    // logic device
    void createLogicalDevice();
    void createHeadlessLogicalDevice();

    // physical device
    void pickPhysicalDevice();
    void pickPhysicalDeviceWithSpecifiedName();
    bool isPhysicalDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void printPhysicalDeviceInfo();

    // validation layer support
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }
}; // SceneViewer

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}