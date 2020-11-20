#include "renderer.h"

namespace {
#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *userData) {
    std::stringstream ss;
    ss << "validation layer: " << pCallbackData->pMessage << "\n";
    if (userData != nullptr) {
        vulkan_proto::Params &params =
            *static_cast<vulkan_proto::Params *>(userData);
        EDBLOG(ss.str().c_str());
    } else {
        fprintf(stderr, ss.str().c_str());
    }
    return false;
}
#endif
} // namespace

namespace vulkan_proto {
void initGLFW(Params &params) {
    LOG("=GLFW init=\n");
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    params.window =
        glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
}

void terminateGLFW(Params &params) {
    LOG("=GLFW terminate=\n");
    if (params.window != nullptr) {
        glfwDestroyWindow(params.window);
    }
    glfwTerminate();
}

void createInstance(Params &params) {
    LOG("=Create instance=\n");
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
    for (uint32_t i = 0; i < params.vkc.validationLayers.size(); i++) {
        bool found = false;
        for (const auto &layerProps : availableLayers) {
            if (strcmp(params.vkc.validationLayers[i], layerProps.layerName) ==
                0) {
                found = true;
                break;
            }
        }
        foundAll &= found;
        if (foundAll == false) {
            break;
        }
    }

    ABORT_IF(foundAll == false, "Not all validation layers were found");

    instanceCi.enabledLayerCount = params.vkc.validationLayers.size();
    instanceCi.ppEnabledLayerNames = params.vkc.validationLayers.data();
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
    if (params.log.verbosity == Verbosity::DEBUG) {
        dbgMsgrCi.messageSeverity |=
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    }
    dbgMsgrCi.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgMsgrCi.pfnUserCallback = debugCallback;
    dbgMsgrCi.pUserData = static_cast<void *>(&params);

    instanceCi.pNext = &dbgMsgrCi;
#endif

    instanceCi.enabledExtensionCount = extensions.size();
    instanceCi.ppEnabledExtensionNames = extensions.data();

    VK_CHECK(
        vkCreateInstance(&instanceCi, params.allocator, &params.vkc.instance));

#ifndef NDEBUG
    LOG("=Create debug messenger=\n");
    auto f = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        params.vkc.instance, "vkCreateDebugUtilsMessengerEXT");
    ABORT_IF(f == nullptr,
             "PFN_vkCreateDebugUtilsMessengerEXT returned nullptr");
    VK_CHECK(f(params.vkc.instance, &dbgMsgrCi, params.allocator,
               &params.vkc.dbgMsgr));
#endif
}

void createDevice(Params &params) {
    LOG("=Create physical device=\n");
    uint32_t deviceCount = 0;
    VK_CHECK(
        vkEnumeratePhysicalDevices(params.vkc.instance, &deviceCount, nullptr));
    ABORT_IF(deviceCount == 0, "No physical devices available");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(params.vkc.instance, &deviceCount,
                                        devices.data()));

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkPresentModeKHR> presentModes;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32_t graphicsFamily = 0;
    uint32_t presentFamily = 0;

    auto checkExtensionSupport = [&params](const VkPhysicalDevice &device) {
        std::vector<VkExtensionProperties> extProps;
        uint32_t extensionCount = 0;
        VK_CHECK(vkEnumerateDeviceExtensionProperties(
            device, nullptr, &extensionCount, nullptr));
        extProps.resize(extensionCount);
        VK_CHECK(vkEnumerateDeviceExtensionProperties(
            device, nullptr, &extensionCount, extProps.data()));

        bool extensionsFound = true;
        for (const auto &requiredExt :
             params.vkc.device.physical.requiredExtensions) {
            bool found = false;
            for (const auto &availExt : extProps) {
                if (strcmp(requiredExt, availExt.extensionName) == 0) {
                    found = true;
                    break;
                }
            }

            extensionsFound &= found;
            if (extensionsFound == false) {
                break;
            }
        }

        return extensionsFound;
    };

    auto checkQueueSupport = [&surfaceCapabilities, &presentModes,
                              &surfaceFormats, &params, &graphicsFamily,
                              &presentFamily](const VkPhysicalDevice &device) {
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            device, params.vkc.surface, &surfaceCapabilities));

        surfaceFormats.clear();
        uint32_t formatCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, params.vkc.surface, &formatCount, nullptr));
        surfaceFormats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, params.vkc.surface, &formatCount, surfaceFormats.data()));

        if (surfaceFormats.empty()) {
            ELOG("Physical device does not support any surface formats,\n"
                 "vkGetPhysicalDeviceSurfaceFormatsKHR returned empty.\n");
            return false;
        }

        presentModes.clear();
        uint32_t modeCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, params.vkc.surface, &modeCount, nullptr));
        presentModes.resize(modeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, params.vkc.surface, &modeCount, presentModes.data()));

        if (presentModes.empty()) {
            ELOG("Physical device does not support any present modes,\n"
                 "vkGetPhysicalDeviceSurfacePresentModesKHR returned empty.\n");
            return false;
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 nullptr);
        std::vector<VkQueueFamilyProperties> qFamProps(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 qFamProps.data());

        int gf = -1;
        int pf = -1;
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            const VkQueueFamilyProperties &qfp = qFamProps[i];
            if (qfp.queueCount > 0) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    device, i, params.vkc.surface, &presentSupport);
                pf = presentSupport ? static_cast<int>(i) : pf;
                gf = (qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0
                         ? static_cast<int>(i)
                         : gf;
            }

            if (gf != -1 && pf != -1) {
                break;
            }
        }

        if (gf == -1 || pf == -1) {
            if (gf == -1) {
                ELOG("The device does not have a queue family with "
                     "VK_QUEUE_GRAPHICS_BIT set\n");
            }

            if (pf == -1) {
                ELOG("The device does not have a queue family with present "
                     "support\n");
            }

            return false;
        }

        // Casting -1 to uint is bad, but that should not happen, as we return
        // early in that case.
        graphicsFamily = static_cast<uint32_t>(gf);
        presentFamily = static_cast<uint32_t>(pf);

        return true;
    };

    auto checkFeatureSupport = [&params](const VkPhysicalDevice &device) {
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);
        return features.geometryShader && features.tessellationShader &&
               features.samplerAnisotropy; 

    };

    auto evaluateDevice = [&deviceCount, &devices, &checkExtensionSupport,
                           &checkQueueSupport, &checkFeatureSupport,
                           &graphicsFamily, &presentFamily, &params,
                           &presentModes, &surfaceFormats,
                           &surfaceCapabilities]() {
        printf("Pick your preferred device and we'll check if that is suitable "
               "for our needs:\n");
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
        // Back to zero based index
        deviceNum -= 1;

        VkPhysicalDeviceProperties devProps;
        vkGetPhysicalDeviceProperties(devices[deviceNum], &devProps);
        LOG("You chose device %d) %s, checking it for compatibility...\n",
            deviceNum + 1, devProps.deviceName);

        if (checkExtensionSupport(devices[deviceNum]) == false) {
            ELOG("Not all required extensions are supported by the "
                 "device.\n");
            return false;
        }

        if (checkQueueSupport(devices[deviceNum]) == false) {
            ELOG("Could not find suitable queues from the "
                 "device.\n");
            return false;
        }

        if (checkFeatureSupport(devices[deviceNum]) == false) {
            ELOG("Not all required features are supported by the "
                 "device.\n");
            return false;
        }

        LOG("Device passed all checks\n");

        params.vkc.device.physical.device = devices[deviceNum];
        params.vkc.device.physical.surfaceCapabilities = surfaceCapabilities;
        params.vkc.device.physical.surfaceFormats = surfaceFormats;
        params.vkc.device.physical.presentModes = presentModes;
        params.vkc.device.physical.grahicsFamily = graphicsFamily;
        params.vkc.device.physical.presentFamily = presentFamily;

        // Cache these for later
        vkGetPhysicalDeviceMemoryProperties(
            params.vkc.device.physical.device,
            &params.vkc.device.physical.memoryProperties);

        return true;
    };

    std::string input;
    while (evaluateDevice() == false) {
        printf("Choose a new device or quit.\nQuit [Y/n]?");
        getline(std::cin, input);
        if (input.size() == 0 ||
            (input.size() == 1 && (input.at(0) == 'y' || input.at(0) == 'Y'))) {
            break;
        }
    }

    LOG("=Create logical device=\n");
    std::vector<VkDeviceQueueCreateInfo> qcis;
    std::set<int> uniQueue = {params.vkc.device.physical.grahicsFamily,
                              params.vkc.device.physical.presentFamily};

    float queuePriority = 1.0f;
    for (int qfi : uniQueue) {
        VkDeviceQueueCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        ci.queueFamilyIndex = qfi;
        ci.queueCount = 1;
        ci.pQueuePriorities = &queuePriority;
        qcis.push_back(ci);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.geometryShader = VK_TRUE;
    deviceFeatures.tessellationShader = VK_TRUE;
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCi = {};
    deviceCi.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCi.pQueueCreateInfos = qcis.data();
    deviceCi.queueCreateInfoCount = static_cast<uint32_t>(qcis.size());
    deviceCi.pEnabledFeatures = &deviceFeatures;
    deviceCi.enabledExtensionCount = static_cast<uint32_t>(
        params.vkc.device.physical.requiredExtensions.size());
    deviceCi.ppEnabledExtensionNames =
        params.vkc.device.physical.requiredExtensions.data();
    deviceCi.enabledLayerCount =
        static_cast<uint32_t>(params.vkc.validationLayers.size());
    deviceCi.ppEnabledLayerNames = params.vkc.validationLayers.data();

    VK_CHECK(vkCreateDevice(params.vkc.device.physical.device, &deviceCi,
                            params.allocator, &params.vkc.device.logical));

    vkGetDeviceQueue(params.vkc.device.logical,
                     params.vkc.device.physical.grahicsFamily, 0,
                     &params.vkc.device.graphicsQueue);
    vkGetDeviceQueue(params.vkc.device.logical,
                     params.vkc.device.physical.presentFamily, 0,
                     &params.vkc.device.presentQueue);

    VkCommandPoolCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = params.vkc.device.physical.grahicsFamily;
    cpci.flags = 0;

    LOG("=Create command pool=\n");
    VK_CHECK(vkCreateCommandPool(params.vkc.device.logical, &cpci,
                                 params.allocator,
                                 &params.vkc.device.commandPool));
}

void init(Params &params) {
    initGLFW(params);
    createInstance(params);
    LOG("=Create surface=\n");
    VK_CHECK(glfwCreateWindowSurface(params.vkc.instance, params.window,
                                     params.allocator, &params.vkc.surface));
    createDevice(params);
}

void terminate(Params &params) {
    if (params.vkc.device.logical != VK_NULL_HANDLE) {
        VK_CHECK(vkDeviceWaitIdle(params.vkc.device.logical));
    }

    if (params.vkc.instance != VK_NULL_HANDLE) {
#ifndef NDEBUG
        LOG("=Destroy debug messenger=\n");
        auto f = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            params.vkc.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (f != nullptr) {
            f(params.vkc.instance, params.vkc.dbgMsgr, params.allocator);
            params.vkc.dbgMsgr = VK_NULL_HANDLE;
        }
#endif
        if (params.vkc.device.logical != VK_NULL_HANDLE) {
            if (params.vkc.device.commandPool != VK_NULL_HANDLE) {
                LOG("=Destroy command pool=\n");
                vkDestroyCommandPool(params.vkc.device.logical,
                                     params.vkc.device.commandPool,
                                     params.allocator);
                params.vkc.device.commandPool = VK_NULL_HANDLE;
            }

            LOG("=Destroy logical device=\n");
            vkDestroyDevice(params.vkc.device.logical, params.allocator);
            params.vkc.device.logical = VK_NULL_HANDLE;
            params.vkc.device.graphicsQueue = VK_NULL_HANDLE;
            params.vkc.device.presentQueue = VK_NULL_HANDLE;
            params.vkc.device.commandPool = VK_NULL_HANDLE;
            params.vkc.device.physical.device = VK_NULL_HANDLE;
        }

        if (params.vkc.surface != VK_NULL_HANDLE) {
            LOG("=Destroy surface=\n");
            vkDestroySurfaceKHR(params.vkc.instance, params.vkc.surface,
                                params.allocator);
            params.vkc.surface = VK_NULL_HANDLE;
        }

        LOG("=Destroy instance=\n");
        vkDestroyInstance(params.vkc.instance, params.allocator);
        params.vkc.instance = VK_NULL_HANDLE;
    }

    terminateGLFW(params);
}

void run() {
    Params params = {};
    params.startingTime = std::chrono::steady_clock::now();

    std::ofstream errStream("err.log", std::ios::out);
    if (errStream.is_open()) {
        params.log.errStream = &errStream;
    }

    init(params);
    terminate(params);
}
} // namespace vulkan_proto
