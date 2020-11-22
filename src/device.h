#pragma once
#include "headers.h"

namespace vulkan_proto {
struct Device {
    VulkanContext_Temp *m_ctx = nullptr;
    VkDevice m_handle = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkPhysicalDevice m_device = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR m_surfCap = {};
    VkPhysicalDeviceMemoryProperties m_memProps = {};
    int m_graphicsFI = -1;
    int m_presentFI = -1;
    std::array<const char *, 1> m_requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    Device(VulkanContext_Temp *ctx);
    ~Device();
    void create();
    void destroy();
};
}
