#pragma once
#include "headers.h"

namespace vulkan_proto {
struct Device {
    VulkanContext *m_ctx = nullptr;
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

    Device();
    ~Device();
    void create(VulkanContext *ctx);
    void destroy();
    void createImage(uint32_t width, uint32_t height, uint32_t depth,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage &image, VkDeviceMemory &imageMemory);
    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout);
    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
};
}
