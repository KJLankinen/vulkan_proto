#pragma once

#include "headers.h"

namespace vulkan_proto {
struct Renderer;
struct Logger;
struct RenderPass {
    const Renderer &m_renderer;
    VkRenderPass m_handle = VK_NULL_HANDLE;

    RenderPass(Renderer &renderer);
    ~RenderPass();
    void create(bool recycle = false);
    void destroy();
    Logger &getLogger();
};
} // namespace vulkan_proto
