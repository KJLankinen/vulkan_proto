#include "renderer.h"

namespace {
VkResult CreateDebugReportCallBackEXT() {}
} // namespace

namespace vulkan_proto {

void initGLFW(Params &params) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    params.window =
        glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
}

void terminateGLFW(Params &params) {
    glfwDestroyWindow(params.window);
    glfwTerminate();
}

void init(Params &params) {
    initGLFW(params);

    // Create instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan prototype";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "nengine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCi = {};
    instanceCi.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCi.pApplicationInfo = &appInfo;

#ifndef NDEBUG
    std::array<const char *, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool foundAll = true;
    for (uint32_t i = 0; i < validationLayers.size(); i++) {
        bool found = false;
        for (const auto &layerProps : availableLayers) {
            if (0 == strcmp(validationLayers[i], layerProps.layerName)) {
                found = true;
                break;
            }
        }
        foundAll &= found;
    }

    assert(foundAll && "Not all validation layers were found.");

    instanceCi.enabledLayerCount = validationLayers.size();
    instanceCi.ppEnabledLayerNames = validationLayers.data();
#endif

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = nullptr;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions,
                                         glfwExtensions + glfwExtensionCount);
#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    instanceCi.enabledExtensionCount = extensions.size();
    instanceCi.ppEnabledExtensionNames = extensions.data();

    VK_ASSERT_CALL(vkCreateInstance(&instanceCi, params.memoryAllocator,
                                    &params.vulkanContext.instance));
}

void terminate(Params &params) {
    if (VK_NULL_HANDLE != params.vulkanContext.device.logical) {
        vkDeviceWaitIdle(params.vulkanContext.device.logical);
    }

    vkDestroyInstance(params.vulkanContext.instance, params.memoryAllocator);
    terminateGLFW(params);
}

void run() {
    Params params = {};
    init(params);

    terminate(params);
}
} // namespace vulkan_proto
