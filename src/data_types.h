#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan_proto{
struct PhysicalDevice {
    VkPhysicalDevice device = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    int graphicsFamily = -1;
    int presentFamily = -1;
    std::array<const char *, 1> requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

struct Device {
    VkDevice logical = VK_NULL_HANDLE;
    PhysicalDevice physical = {};
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
};

struct Swapchain {
    VkSwapchainKHR chain = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    VkImageView depthImageView = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    VkSurfaceFormatKHR surfaceFormat = {};
    VkExtent2D extent = {};
};

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    Device device = {};
    Swapchain swapchain = {};
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT dbgMsgr = VK_NULL_HANDLE;
    std::array<const char *, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkAllocationCallbacks *allocator = nullptr;
};

struct Params {
    VulkanContext vkc = {};
    std::ofstream *fileStream = nullptr;
    std::chrono::steady_clock::time_point startingTime;
    std::stringstream timess;
    GLFWwindow *window = nullptr;
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    bool recreateSwapchain = false;
};
}
