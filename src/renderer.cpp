#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <stdio.h>
#include <vulkan/vulkan.h>

namespace vulkan_proto {
struct Context {
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};

void run() {
    Context context;
    printf("Context created succesfully.\n");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window =
        glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t nExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &nExtensions, nullptr);
    printf("# extensions: %d\n", nExtensions);

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
} // namespace vulkan_proto
