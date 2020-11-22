#pragma once

#include "headers.h"

namespace vulkan_proto {
struct Swapchain {
    VulkanContext_Temp *m_ctx = nullptr;
    VkSwapchainKHR m_handle = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_views;

    VkImageView m_depthView = VK_NULL_HANDLE;
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
    VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;

    VkSurfaceFormatKHR m_surfaceFormat = {};
    VkExtent2D m_extent = {};

    Swapchain(VulkanContext_Temp *ctx);
    ~Swapchain();
    void create(bool recycle);
    void destroy(VkSwapchainKHR chain);
    void chooseFormats();
};
}
