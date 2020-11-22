#pragma once

#include "headers.h"

namespace vulkan_proto {
struct RenderPass {
    VulkanContext_Temp *m_ctx = nullptr;
    VkRenderPass m_handle = VK_NULL_HANDLE;

    RenderPass(VulkanContext_Temp *ctx);
    ~RenderPass();
    void create(bool recycle);
    void destroy();
};
} // namespace vulkan_proto
