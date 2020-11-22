#include "surface.h"
#include "instance.h"
#include "renderer.h"

namespace vulkan_proto {
Surface::Surface(Renderer &renderer) : m_renderer(renderer) {}
Surface::~Surface() {}

void Surface::create() {
    LOG("=Create surface=");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Vulkan window",
                                nullptr, nullptr);

    VK_CHECK(glfwCreateWindowSurface(m_renderer.getInstance(), m_window,
                                     m_renderer.getAllocator(), &m_handle));
}

void Surface::destroy() {
    LOG("=Destroy surface=");
    vkDestroySurfaceKHR(m_renderer.getInstance(), m_handle,
                        m_renderer.getAllocator());
    m_handle = VK_NULL_HANDLE;

    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
    }
}

void Surface::initWindow() {
    LOG("=Init window=");
    glfwInit();
}

void Surface::terminateWindow() {
    LOG("=Terminate window=");
    glfwTerminate();
}

Logger &Surface::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
