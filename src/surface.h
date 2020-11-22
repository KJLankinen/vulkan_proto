#pragma once
#include "headers.h"

namespace vulkan_proto {
struct Renderer;
struct Logger;
struct Surface {
    const Renderer &m_renderer;
    VkSurfaceKHR m_handle = VK_NULL_HANDLE;

    GLFWwindow *m_window = nullptr;
    uint32_t m_windowWidth = 800;
    uint32_t m_windowHeight = 600;

    Surface(Renderer &renderer);
    ~Surface();
    void create();
    void destroy();
    void initWindow();
    void terminateWindow();
    Logger &getLogger();
};
}
