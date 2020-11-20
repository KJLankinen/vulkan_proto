#include "renderer.h"

namespace {
#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *userData) {
    if (userData != nullptr) {
        vulkan_proto::Params &params =
            *static_cast<vulkan_proto::Params *>(userData);
        LOG("validation layer: %s", pCallbackData->pMessage);
    } else {
        fprintf(stderr, "validation layer: %s", pCallbackData->pMessage);
    }
    return false;
}
#endif
} // namespace

namespace vulkan_proto {
void createInstance(Params &params) {
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
    // if (params.log.verbosity == Verbosity::DEBUG) {
    //    dbgMsgrCi.messageSeverity |=
    //        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    //}
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
    LOG("=Create debug messenger=");
    auto f = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        params.vkc.instance, "vkCreateDebugUtilsMessengerEXT");
    ABORT_IF(f == nullptr,
             "PFN_vkCreateDebugUtilsMessengerEXT returned nullptr");
    VK_CHECK(f(params.vkc.instance, &dbgMsgrCi, params.allocator,
               &params.vkc.dbgMsgr));
#endif
}

void createDevice(Params &params) {
    LOG("=Create physical device=");
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
            LOG("Physical device does not support any surface formats,\n"
                "vkGetPhysicalDeviceSurfaceFormatsKHR returned empty.");
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
            LOG("Physical device does not support any present modes,\n"
                "vkGetPhysicalDeviceSurfacePresentModesKHR returned empty.");
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
                LOG("The device does not have a queue family with "
                    "VK_QUEUE_GRAPHICS_BIT set");
            }

            if (pf == -1) {
                LOG("The device does not have a queue family with present "
                    "support");
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
        LOG("You chose device %d) %s, checking it for compatibility...",
            deviceNum + 1, devProps.deviceName);

        if (checkExtensionSupport(devices[deviceNum]) == false) {
            LOG("Not all required extensions are supported by the "
                "device.");
            return false;
        }

        if (checkQueueSupport(devices[deviceNum]) == false) {
            LOG("Could not find suitable queues from the "
                "device.");
            return false;
        }

        if (checkFeatureSupport(devices[deviceNum]) == false) {
            LOG("Not all required features are supported by the "
                "device.");
            return false;
        }

        LOG("Device passed all checks");

        params.vkc.device.physical.device = devices[deviceNum];
        params.vkc.device.physical.surfaceCapabilities = surfaceCapabilities;
        params.vkc.device.physical.surfaceFormats = surfaceFormats;
        params.vkc.device.physical.presentModes = presentModes;
        params.vkc.device.physical.graphicsFamily = graphicsFamily;
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

    LOG("=Create logical device=");
    std::vector<VkDeviceQueueCreateInfo> qcis;
    std::set<int> uniQueue = {params.vkc.device.physical.graphicsFamily,
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
                     params.vkc.device.physical.graphicsFamily, 0,
                     &params.vkc.device.graphicsQueue);
    vkGetDeviceQueue(params.vkc.device.logical,
                     params.vkc.device.physical.presentFamily, 0,
                     &params.vkc.device.presentQueue);

    VkCommandPoolCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = params.vkc.device.physical.graphicsFamily;
    cpci.flags = 0;

    LOG("=Create command pool=");
    VK_CHECK(vkCreateCommandPool(params.vkc.device.logical, &cpci,
                                 params.allocator,
                                 &params.vkc.device.commandPool));
}

void pickSwapchainFormats(Params &params) {
    LOG("=Choose swapchain formats=");
    params.vkc.swapchain.surfaceFormat =
        params.vkc.device.physical.surfaceFormats[0];

    if (params.vkc.device.physical.surfaceFormats.size() == 1 &&
        params.vkc.device.physical.surfaceFormats[0].format ==
            VK_FORMAT_UNDEFINED)
        params.vkc.swapchain.surfaceFormat = {
            VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    else {
        for (const auto &availableFormat :
             params.vkc.device.physical.surfaceFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace ==
                    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                params.vkc.swapchain.surfaceFormat = availableFormat;
                break;
            }
        }
    }

    const std::array<VkFormat, 3> candidates = {VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                VK_FORMAT_D24_UNORM_S8_UINT};
    const VkFormatFeatureFlags features =
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (auto &format : candidates) {
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(params.vkc.device.physical.device,
                                            format, &props);

        if ((props.optimalTilingFeatures & features) == features) {
            params.vkc.swapchain.depthFormat = format;
            break;
        }
    }

    ABORT_IF(params.vkc.swapchain.depthFormat == VK_FORMAT_UNDEFINED,
             "Failed to find a supported format!");
}

void createRenderPass(Params &params) {
    LOG("=Create render pass=");
    VkAttachmentDescription colorAttchDes = {};
    colorAttchDes.flags = 0;
    colorAttchDes.format = params.vkc.swapchain.surfaceFormat.format;
    colorAttchDes.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttchDes.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttchDes.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttchDes.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttchDes.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttchDes.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttchDes.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttchDes = {};
    depthAttchDes.flags = 0;
    depthAttchDes.format = params.vkc.swapchain.depthFormat;
    depthAttchDes.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttchDes.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttchDes.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttchDes.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttchDes.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttchDes.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttchDes.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttchDes,
                                                          depthAttchDes};

    VkAttachmentReference colorAttchRef = {};
    colorAttchRef.attachment = 0;
    colorAttchRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttchRef = {};
    depthAttchRef.attachment = 1;
    depthAttchRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttchRef;
    subpass.pDepthStencilAttachment = &depthAttchRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCi = {};
    renderPassCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCi.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCi.pAttachments = attachments.data();
    renderPassCi.subpassCount = 1;
    renderPassCi.pSubpasses = &subpass;
    renderPassCi.dependencyCount = 1;
    renderPassCi.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(params.vkc.device.logical, &renderPassCi,
                                params.allocator, &params.vkc.renderPass));
}

void createSwapchain(Params &params) {
    LOG("=Create swap chain=");
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto &availablePresentMode :
         params.vkc.device.physical.presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }

    if (params.vkc.device.physical.surfaceCapabilities.currentExtent.width !=
            0xFFFFFFFF &&
        params.vkc.device.physical.surfaceCapabilities.currentExtent.height !=
            0xFFFFFFFF) {
        params.vkc.swapchain.extent =
            params.vkc.device.physical.surfaceCapabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {params.windowWidth, params.windowHeight};

        actualExtent.width = std::max(
            params.vkc.device.physical.surfaceCapabilities.minImageExtent.width,
            std::min(params.vkc.device.physical.surfaceCapabilities
                         .maxImageExtent.width,
                     actualExtent.width));

        actualExtent.height =
            std::max(params.vkc.device.physical.surfaceCapabilities
                         .minImageExtent.height,
                     std::min(params.vkc.device.physical.surfaceCapabilities
                                  .maxImageExtent.height,
                              actualExtent.height));

        params.vkc.swapchain.extent = actualExtent;
    }

    uint32_t imageCount =
        params.vkc.device.physical.surfaceCapabilities.minImageCount + 1;
    if (params.vkc.device.physical.surfaceCapabilities.maxImageCount > 0 &&
        imageCount >
            params.vkc.device.physical.surfaceCapabilities.maxImageCount) {
        imageCount =
            params.vkc.device.physical.surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCi = {};
    swapchainCi.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCi.surface = params.vkc.surface;
    swapchainCi.minImageCount = imageCount;
    swapchainCi.imageFormat = params.vkc.swapchain.surfaceFormat.format;
    swapchainCi.imageColorSpace = params.vkc.swapchain.surfaceFormat.colorSpace;
    swapchainCi.imageExtent = params.vkc.swapchain.extent;
    swapchainCi.imageArrayLayers = 1;
    swapchainCi.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (params.vkc.device.physical.graphicsFamily !=
        params.vkc.device.physical.presentFamily) {
        const uint32_t queueFamilyIndices[] = {
            (uint32_t)params.vkc.device.physical.graphicsFamily,
            (uint32_t)params.vkc.device.physical.presentFamily};

        swapchainCi.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCi.queueFamilyIndexCount = 2;
        swapchainCi.pQueueFamilyIndices = queueFamilyIndices;
    } else
        swapchainCi.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    swapchainCi.preTransform =
        params.vkc.device.physical.surfaceCapabilities.currentTransform;
    swapchainCi.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCi.presentMode = presentMode;
    swapchainCi.clipped = VK_TRUE;

    VkSwapchainKHR oldChain = params.vkc.swapchain.chain;
    swapchainCi.oldSwapchain = oldChain;

    VkSwapchainKHR newChain = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSwapchainKHR(params.vkc.device.logical, &swapchainCi,
                                  params.allocator, &newChain));
    params.vkc.swapchain.chain = newChain;

    if (params.recreateSwapchain) {
        for (uint32_t i = 0;
             i < static_cast<uint32_t>(params.vkc.swapchain.images.size());
             ++i) {
            vkDestroyImageView(params.vkc.device.logical,
                               params.vkc.swapchain.imageViews[i],
                               params.allocator);
            vkDestroyFramebuffer(params.vkc.device.logical,
                                 params.vkc.swapchain.framebuffers[i],
                                 params.allocator);
        }

        vkDestroySwapchainKHR(params.vkc.device.logical, oldChain,
                              params.allocator);

        params.vkc.swapchain.images.clear();
        params.vkc.swapchain.imageViews.clear();
        params.vkc.swapchain.framebuffers.clear();

        vkDestroyImageView(params.vkc.device.logical,
                           params.vkc.swapchain.depthImageView,
                           params.allocator);
        params.vkc.swapchain.depthImageView = VK_NULL_HANDLE;

        vkDestroyImage(params.vkc.device.logical,
                       params.vkc.swapchain.depthImage, params.allocator);
        params.vkc.swapchain.depthImage = VK_NULL_HANDLE;

        vkFreeMemory(params.vkc.device.logical,
                     params.vkc.swapchain.depthImageMemory, params.allocator);
        params.vkc.swapchain.depthImageMemory = VK_NULL_HANDLE;
    }

    // Get images
    imageCount = 0;
    vkGetSwapchainImagesKHR(params.vkc.device.logical,
                            params.vkc.swapchain.chain, &imageCount, nullptr);
    params.vkc.swapchain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(params.vkc.device.logical,
                            params.vkc.swapchain.chain, &imageCount,
                            params.vkc.swapchain.images.data());

    // Create image views
    params.vkc.swapchain.imageViews.resize(imageCount);

    VkImageViewCreateInfo imageViewCi = {};
    imageViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCi.format = params.vkc.swapchain.surfaceFormat.format;
    imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCi.subresourceRange.baseMipLevel = 0;
    imageViewCi.subresourceRange.levelCount = 1;
    imageViewCi.subresourceRange.baseArrayLayer = 0;
    imageViewCi.subresourceRange.layerCount = 1;

    for (uint32_t i = 0; i < imageCount; ++i) {
        imageViewCi.image = params.vkc.swapchain.images[i];
        VK_CHECK(vkCreateImageView(params.vkc.device.logical, &imageViewCi,
                                   params.allocator,
                                   &params.vkc.swapchain.imageViews[i]));
    }

    // Depth
    createImage(
        params, params.vkc.swapchain.extent.width,
        params.vkc.swapchain.extent.height, 1, params.vkc.swapchain.depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, params.vkc.swapchain.depthImage,
        params.vkc.swapchain.depthImageMemory);

    imageViewCi = {};
    imageViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCi.image = params.vkc.swapchain.depthImage;
    imageViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCi.format = params.vkc.swapchain.depthFormat;
    imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewCi.subresourceRange.baseMipLevel = 0;
    imageViewCi.subresourceRange.levelCount = 1;
    imageViewCi.subresourceRange.baseArrayLayer = 0;
    imageViewCi.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(params.vkc.device.logical, &imageViewCi,
                               params.allocator,
                               &params.vkc.swapchain.depthImageView));

    transitionImageLayout(params, params.vkc.swapchain.depthImage,
                          params.vkc.swapchain.depthFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    // Framebuffers
    VkFramebufferCreateInfo framebufferCi = {};
    framebufferCi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCi.renderPass = params.vkc.renderPass;
    framebufferCi.width = params.vkc.swapchain.extent.width;
    framebufferCi.height = params.vkc.swapchain.extent.height;
    framebufferCi.layers = 1;
    framebufferCi.attachmentCount = 2;

    params.vkc.swapchain.framebuffers.resize(
        params.vkc.swapchain.images.size());
    for (size_t i = 0; i < params.vkc.swapchain.framebuffers.size(); i++) {
        const VkImageView attachments[] = {params.vkc.swapchain.imageViews[i],
                                           params.vkc.swapchain.depthImageView};
        framebufferCi.pAttachments = attachments;

        VK_CHECK(vkCreateFramebuffer(params.vkc.device.logical, &framebufferCi,
                                     params.allocator,
                                     &params.vkc.swapchain.framebuffers[i]));
    }
}

void createTexturSampler(Params &params) {
    LOG("=Create texture sampler=");
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy = 16;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 0.0f;

    VK_CHECK(vkCreateSampler(params.vkc.device.logical, &info, params.allocator,
                             &params.vkc.textureSampler));
}

void createImage(Params &params, uint32_t width, uint32_t height,
                 uint32_t depth, VkFormat format, VkImageTiling tiling,
                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = depth;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    VK_CHECK(vkCreateImage(params.vkc.device.logical, &imageInfo,
                           params.allocator, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(params.vkc.device.logical, image,
                                 &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(params, memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(params.vkc.device.logical, &allocInfo, nullptr,
                              &imageMemory));

    VK_CHECK(
        vkBindImageMemory(params.vkc.device.logical, image, imageMemory, 0));
}

uint32_t findMemoryType(Params &params, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0;
         i < params.vkc.device.physical.memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) &&
            (params.vkc.device.physical.memoryProperties.memoryTypes[i]
                 .propertyFlags &
             properties) == properties) {
            return i;
        }
    }

    ABORT_IF(true, "Failed to find suitable memory type!");

    return ~0u;
}

void transitionImageLayout(Params &params, VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(params);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        ABORT_IF(true, "Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(params, commandBuffer);
}

VkCommandBuffer beginSingleTimeCommands(Params &params) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = params.vkc.device.commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(params.vkc.device.logical, &allocInfo,
                                      &commandBuffer));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return commandBuffer;
}

void endSingleTimeCommands(Params &params, VkCommandBuffer commandBuffer) {
    if (commandBuffer != VK_NULL_HANDLE) {
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VK_CHECK(vkQueueSubmit(params.vkc.device.graphicsQueue, 1, &submitInfo,
                               VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(params.vkc.device.graphicsQueue));

        vkFreeCommandBuffers(params.vkc.device.logical,
                             params.vkc.device.commandPool, 1, &commandBuffer);
    }
}

void init(Params &params) {
    initGLFW(params);
    createInstance(params);
    LOG("=Create surface=");
    VK_CHECK(glfwCreateWindowSurface(params.vkc.instance, params.window,
                                     params.allocator, &params.vkc.surface));
    createDevice(params);
    pickSwapchainFormats(params);
    createRenderPass(params);
    createSwapchain(params);
    createTexturSampler(params);
}

void terminate(Params &params) {
    if (params.vkc.device.logical != VK_NULL_HANDLE) {
        VK_CHECK(vkDeviceWaitIdle(params.vkc.device.logical));
    }

    LOG("=Destroy texture sampler=");
    vkDestroySampler(params.vkc.device.logical, params.vkc.textureSampler,
                     params.allocator);

    LOG("=Destroy swap chain=");
    vkDestroyImageView(params.vkc.device.logical,
                       params.vkc.swapchain.depthImageView, params.allocator);
    params.vkc.swapchain.depthImageView = VK_NULL_HANDLE;

    vkDestroyImage(params.vkc.device.logical, params.vkc.swapchain.depthImage,
                   params.allocator);
    params.vkc.swapchain.depthImage = VK_NULL_HANDLE;

    vkFreeMemory(params.vkc.device.logical,
                 params.vkc.swapchain.depthImageMemory, params.allocator);
    params.vkc.swapchain.depthImageMemory = VK_NULL_HANDLE;

    for (auto &iv : params.vkc.swapchain.imageViews) {
        vkDestroyImageView(params.vkc.device.logical, iv, params.allocator);
    }
    params.vkc.swapchain.imageViews.clear();

    for (auto &fb : params.vkc.swapchain.framebuffers) {
        vkDestroyFramebuffer(params.vkc.device.logical, fb, params.allocator);
    }
    params.vkc.swapchain.framebuffers.clear();

    vkDestroySwapchainKHR(params.vkc.device.logical, params.vkc.swapchain.chain,
                          params.allocator);

    LOG("=Destroy render pass=");
    vkDestroyRenderPass(params.vkc.device.logical, params.vkc.renderPass,
                        params.allocator);
    params.vkc.renderPass = VK_NULL_HANDLE;

    LOG("=Destroy command pool=");
    vkDestroyCommandPool(params.vkc.device.logical,
                         params.vkc.device.commandPool, params.allocator);
    params.vkc.device.commandPool = VK_NULL_HANDLE;

    LOG("=Destroy logical device=");
    vkDestroyDevice(params.vkc.device.logical, params.allocator);
    params.vkc.device.logical = VK_NULL_HANDLE;
    params.vkc.device.graphicsQueue = VK_NULL_HANDLE;
    params.vkc.device.presentQueue = VK_NULL_HANDLE;
    params.vkc.device.commandPool = VK_NULL_HANDLE;
    params.vkc.device.physical.device = VK_NULL_HANDLE;

    LOG("=Destroy surface=");
    vkDestroySurfaceKHR(params.vkc.instance, params.vkc.surface,
                        params.allocator);
    params.vkc.surface = VK_NULL_HANDLE;

#ifndef NDEBUG
    LOG("=Destroy debug messenger=");
    auto f = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        params.vkc.instance, "vkDestroyDebugUtilsMessengerEXT");
    if (f != nullptr) {
        f(params.vkc.instance, params.vkc.dbgMsgr, params.allocator);
        params.vkc.dbgMsgr = VK_NULL_HANDLE;
    }
#endif

    LOG("=Destroy instance=");
    vkDestroyInstance(params.vkc.instance, params.allocator);
    params.vkc.instance = VK_NULL_HANDLE;

    terminateGLFW(params);
}

void initGLFW(Params &params) {
    LOG("=GLFW init=");
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    params.window = glfwCreateWindow(params.windowWidth, params.windowHeight,
                                     "Vulkan window", nullptr, nullptr);
}

void terminateGLFW(Params &params) {
    LOG("=GLFW terminate=");
    if (params.window != nullptr) {
        glfwDestroyWindow(params.window);
    }
    glfwTerminate();
}

void run() {
    Params params = {};
#ifndef NDEBUG
    params.startingTime = std::chrono::steady_clock::now();

    std::ofstream fileStream("err.log", std::ios::out);
    if (fileStream.is_open()) {
        params.fileStream = &fileStream;
    }
#endif
    init(params);
    terminate(params);
}
} // namespace vulkan_proto
