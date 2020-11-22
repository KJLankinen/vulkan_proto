#pragma once
#include "headers.h"

namespace vulkan_proto {
struct Surface {
    VulkanContext_Temp *m_ctx = nullptr;
    VkSurfaceKHR m_handle = VK_NULL_HANDLE;

    Surface(VulkanContext_Temp *ctx);
    ~Surface();
    void create();
    void destroy();
};
}
