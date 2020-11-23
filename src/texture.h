#pragma once

#include "headers.h"

namespace vulkan_proto {

struct Renderer;
struct Logger;

struct Texture {
    const Renderer &m_renderer;

    VkImageView m_view = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkDescriptorImageInfo m_descriptor = {};

    Texture(const Renderer &renderer);
    ~Texture();
    void create(const char *filename);
    void destroy();
    Logger &getLogger();
};
} // namespace vulkan_proto
