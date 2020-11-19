#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan_proto{
struct PhysicalDevice {
    VkPhysicalDevice device = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR surfaceCapabilites = {};
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    int grahicsFamily = -1;
    int presenFamily = -1;
};

struct Device {
    VkDevice logical = VK_NULL_HANDLE;
    PhysicalDevice physical = {};
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
};

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    Device device;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};

struct Params {
    VulkanContext vulkanContext;
    GLFWwindow *window = nullptr;
    VkAllocationCallbacks *memoryAllocator = nullptr;
};
}
