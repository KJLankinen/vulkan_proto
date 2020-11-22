#pragma once

#include "headers.h"

namespace vulkan_proto {
struct Renderer;
struct Logger;
struct Swapchain {
    const Renderer &m_renderer;
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

    Swapchain(Renderer &renderer);
    ~Swapchain();
    void create(bool recycle = false);
    void destroy(VkSwapchainKHR chain);
    void chooseFormats();
    Logger &getLogger();
};
}
