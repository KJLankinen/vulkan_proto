#include "surface.h"
#include "instance.h"

namespace vulkan_proto {
Surface::Surface(VulkanContext_Temp *ctx) : m_ctx(ctx) {}
Surface::~Surface() {}

void Surface::create() {
    LOG("=Create surface=");
    VK_CHECK(glfwCreateWindowSurface(m_ctx->instance->m_handle, m_ctx->window,
                                     m_ctx->allocator, &m_handle));
}

void Surface::destroy() {
    LOG("=Destroy surface=");
    vkDestroySurfaceKHR(m_ctx->instance->m_handle, m_handle, m_ctx->allocator);
    m_handle = VK_NULL_HANDLE;
}
} // namespace vulkan_proto
