#include "surface.h"
#include "instance.h"

namespace vulkan_proto {
Surface::Surface(VulkanContext_Temp *ctx) : m_ctx(ctx) {}
Surface::~Surface() {}

void Surface::create() {
    LOG("=Create surface=");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Vulkan window",
                                nullptr, nullptr);

    VK_CHECK(glfwCreateWindowSurface(m_ctx->instance->m_handle, m_window,
                                     m_ctx->allocator, &m_handle));
    m_ctx->surface = this;
}

void Surface::destroy() {
    LOG("=Destroy surface=");
    vkDestroySurfaceKHR(m_ctx->instance->m_handle, m_handle, m_ctx->allocator);
    m_handle = VK_NULL_HANDLE;

    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
    }

    m_ctx->surface = nullptr;
}

void Surface::initWindow() {
    LOG("=Init GLFW=");
    glfwInit();
}

void Surface::terminateWindow() {
    LOG("=Terminate GLFW=");
    glfwTerminate();
}
} // namespace vulkan_proto
