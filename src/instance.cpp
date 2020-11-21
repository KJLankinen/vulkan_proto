#include "instance.h"

namespace {
#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *userData) {
    // if (userData != nullptr) {
    //    vulkan_proto::Log &log = *static_cast<vulkan_proto::Log *>(userData);
    //    LOG("validation layer: %s", pCallbackData->pMessage);
    //} else {
    //    fprintf(stderr, "validation layer: %s", pCallbackData->pMessage);
    //}
    fprintf(stderr, "validation layer: %s", pCallbackData->pMessage);
    return false;
}
#endif
} // namespace

namespace vulkan_proto {
Instance::Instance(VulkanContext_Temp *ctx) : m_ctx(ctx) {}

Instance::~Instance() {}

void Instance::create() {
    LOG("=Create instance=");
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
    std::vector<VkLayerProperties> availableLayers;
    uint32_t layerCount = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
    availableLayers.resize(layerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount,
                                                availableLayers.data()));

    bool foundAll = true;
    for (uint32_t i = 0; i < m_validationLayers.size(); i++) {
        bool found = false;
        for (const auto &layerProps : availableLayers) {
            if (strcmp(m_validationLayers[i], layerProps.layerName) == 0) {
                found = true;
                break;
            }
        }
        foundAll &= found;
        if (foundAll == false) {
            break;
        }
    }

    THROW_IF(foundAll == false, "Not all validation layers were found");

    instanceCi.enabledLayerCount = m_validationLayers.size();
    instanceCi.ppEnabledLayerNames = m_validationLayers.data();
#endif

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = nullptr;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions,
                                         glfwExtensions + glfwExtensionCount);
#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Add call back for instance creation
    VkDebugUtilsMessengerCreateInfoEXT dbgMsgrCi = {};
    dbgMsgrCi.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgMsgrCi.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgMsgrCi.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgMsgrCi.pfnUserCallback = debugCallback;
    dbgMsgrCi.pUserData = nullptr; // static_cast<void *>(&log);

    instanceCi.pNext = &dbgMsgrCi;
#endif

    instanceCi.enabledExtensionCount = extensions.size();
    instanceCi.ppEnabledExtensionNames = extensions.data();

    VK_CHECK(vkCreateInstance(&instanceCi, m_ctx->allocator, &m_handle));
    m_ctx->instance = this;

#ifndef NDEBUG
    LOG("=Create debug messenger=");
    auto f = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_handle, "vkCreateDebugUtilsMessengerEXT");
    THROW_IF(f == nullptr,
             "PFN_vkCreateDebugUtilsMessengerEXT returned nullptr");
    VK_CHECK(f(m_handle, &dbgMsgrCi, m_ctx->allocator, &m_dbgMsgr));
#endif
}

void instance::destroy() {
#ifndef NDEBUG
    LOG("=Destroy debug messenger=");
    auto f = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_handle, "vkDestroyDebugUtilsMessengerEXT");
    if (f != nullptr) {
        f(m_handle, m_dbgMsgr, m_ctx->allocator);
        m_dbgMsgr = VK_NULL_HANDLE;
    }
#endif

    LOG("=Destroy instance=");
    vkDestroyInstance(m_handle, m_ctx->allocator);
    m_handle = VK_NULL_HANDLE;
}
} // namespace vulkan_proto
