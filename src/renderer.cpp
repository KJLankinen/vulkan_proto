#include "renderer.h"

namespace {
#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *userData) {
    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);
    return false;
}
#endif
} // namespace

namespace vulkan_proto {
void initGLFW(Params &params) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    params.window =
        glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
}

void terminateGLFW(Params &params) {
    if (params.window != nullptr) {
        glfwDestroyWindow(params.window);
    }
    glfwTerminate();
}

void createInstance(Params &params) {
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
            if (strcmp(validationLayers[i], layerProps.layerName) == 0) {
                found = true;
                break;
            }
        }
        foundAll &= found;
    }

    ABORT_IF(foundAll == false, "Not all validation layers were found");

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

    // Add call back for instance creation
    VkDebugUtilsMessengerCreateInfoEXT dbgMsgrCi = {};
    dbgMsgrCi.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgMsgrCi.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgMsgrCi.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgMsgrCi.pfnUserCallback = debugCallback;
    dbgMsgrCi.pUserData = nullptr;

    instanceCi.pNext = &dbgMsgrCi;
#endif

    instanceCi.enabledExtensionCount = extensions.size();
    instanceCi.ppEnabledExtensionNames = extensions.data();

    VK_ASSERT_CALL(
        vkCreateInstance(&instanceCi, params.allocator, &params.vkc.instance));

#ifndef NDEBUG
    auto f = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        params.vkc.instance, "vkCreateDebugUtilsMessengerEXT");
    ABORT_IF(f == nullptr,
             "PFN_vkCreateDebugUtilsMessengerEXT returned nullptr");
    VK_ASSERT_CALL(f(params.vkc.instance, &dbgMsgrCi, params.allocator,
                     &params.vkc.dbgMsgr));
#endif
}

void createDevice(Params &params) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(params.vkc.instance, &deviceCount, nullptr);
    ABORT_IF(deviceCount == 0, "No physical devices available");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(params.vkc.instance, &deviceCount,
                               devices.data());

    auto evaluateDevice = [&deviceCount, &devices]() {
        printf("Pick your preferred device and we'll check if that is suitable "
               "for "
               "our needs:\n");
        uint32_t i = 0;
        for (const auto &device : devices) {
            VkPhysicalDeviceProperties devProps;
            vkGetPhysicalDeviceProperties(device, &devProps);
            printf("%d) %s\n", ++i, devProps.deviceName);
        }

        int deviceNum = -1;
        std::string input;
        getline(std::cin, input);
        std::stringstream(input) >> deviceNum;

        while (deviceNum < 1 || deviceNum > static_cast<int>(deviceCount)) {
            printf("Bad input, try again\n");
            getline(std::cin, input);
            std::stringstream(input) >> deviceNum;

            if (deviceNum > static_cast<int>(deviceCount)) {
                deviceNum = -1;
                printf("Choose a device from the given list\n");
            }
        }

        printf("You chose device %d, checking it for compatibility...\n",
               deviceNum);

        return true;
    };

    std::string input;
    while (evaluateDevice() == false) {
        printf("Quit [Y/n]?");
        getline(std::cin, input);
        if (input.size() == 0 ||
            (input.size() == 1 && (input.at(0) == 'y' || input.at(0) == 'Y'))) {
            break;
        }
    }
}

void init(Params &params) {
    initGLFW(params);
    createInstance(params);

    // Create surface
    VK_ASSERT_CALL(glfwCreateWindowSurface(params.vkc.instance, params.window,
                                           params.allocator,
                                           &params.vkc.surface));
    createDevice(params);
}

void terminate(Params &params) {
    if (params.vkc.device.logical != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(params.vkc.device.logical);
    }

    if (params.vkc.instance != VK_NULL_HANDLE) {
#ifndef NDEBUG
        auto f = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            params.vkc.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (f != nullptr) {
            f(params.vkc.instance, params.vkc.dbgMsgr, params.allocator);
        }
#endif

        if (params.vkc.surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(params.vkc.instance, params.vkc.surface,
                                params.allocator);
        }
        vkDestroyInstance(params.vkc.instance, params.allocator);
    }

    terminateGLFW(params);
}

void run() {
    Params params = {};
    init(params);
    terminate(params);
}

void abortIf(Params &params, bool condition, const char *msg, const char *file,
             int line) {
    if (condition) {
        fprintf(stderr, "'%s' at %s:%d\n", msg, file, line);
        fprintf(stderr, "Aborting\n");
        terminate(params);
        exit(EXIT_FAILURE);
    }
}
} // namespace vulkan_proto
