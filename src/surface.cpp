#include "surface.h"
#include "instance.h"
#include "renderer.h"

namespace vulkan_proto {
Surface::Surface(Renderer &renderer) : m_renderer(renderer) {}
Surface::~Surface() {}

void Surface::create() {
    LOG("=Create surface=");
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Vulkan window",
                                nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, static_cast<void *>(this));
    glfwSetFramebufferSizeCallback(m_window, windowResizeCallback);

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
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
}

void Surface::terminateWindow() {
    LOG("=Terminate window=");
    glfwTerminate();
}

void Surface::windowResizeCallback(GLFWwindow *window, int width, int height) {
    Surface *surface = static_cast<Surface *>(glfwGetWindowUserPointer(window));
    if (surface != nullptr) {
        surface->m_windowWidth = width;
        surface->m_windowHeight = height;
        surface->m_renderer.windowResized();
    }
}

Logger &Surface::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
