#pragma once
#include "headers.h"

namespace vulkan_proto {
struct Surface {
    VulkanContext *m_ctx = nullptr;
    VkSurfaceKHR m_handle = VK_NULL_HANDLE;

    GLFWwindow *m_window = nullptr;
    uint32_t m_windowWidth = 800;
    uint32_t m_windowHeight = 600;

    Surface();
    ~Surface();
    void create(VulkanContext *ctx);
    void destroy();
    void initWindow();
    void terminateWindow();
};
}
