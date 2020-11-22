#pragma once

#include "headers.h"

namespace vulkan_proto {
struct RenderPass {
    VulkanContext_Temp *m_ctx = nullptr;
    VkRenderPass m_handle = VK_NULL_HANDLE;

    RenderPass();
    ~RenderPass();
    void create(VulkanContext *ctx, bool recycle = false);
    void destroy();
};
} // namespace vulkan_proto
