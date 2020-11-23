#include "device.h"
#include "instance.h"
#include "renderer.h"

namespace vulkan_proto {
Device::Device(Renderer &renderer) : m_renderer(renderer) {}
Device::~Device() {}

void Device::create() {
    LOG("=Create physical device=");
    uint32_t deviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(m_renderer.getInstance(), &deviceCount,
                                        nullptr));
    THROW_IF(deviceCount == 0, "No physical devices available");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(m_renderer.getInstance(), &deviceCount,
                                        devices.data()));

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkPresentModeKHR> presentModes;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32_t graphicsFamily = 0;
    uint32_t presentFamily = 0;

    auto checkExtensionSupport = [this](const VkPhysicalDevice &device) {
        std::vector<VkExtensionProperties> extProps;
        uint32_t extensionCount = 0;
        VK_CHECK(vkEnumerateDeviceExtensionProperties(
            device, nullptr, &extensionCount, nullptr));
        extProps.resize(extensionCount);
        VK_CHECK(vkEnumerateDeviceExtensionProperties(
            device, nullptr, &extensionCount, extProps.data()));

        bool extensionsFound = true;
        for (const auto &requiredExt : m_requiredExtensions) {
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
                              &surfaceFormats, &graphicsFamily, &presentFamily,
                              this](const VkPhysicalDevice &device) {
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            device, m_renderer.getSurface(), &surfaceCapabilities));

        surfaceFormats.clear();
        uint32_t formatCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, m_renderer.getSurface(), &formatCount, nullptr));
        surfaceFormats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, m_renderer.getSurface(), &formatCount,
            surfaceFormats.data()));

        if (surfaceFormats.empty()) {
            LOG("Physical device does not support any surface formats,\n"
                "vkGetPhysicalDeviceSurfaceFormatsKHR returned empty.");
            return false;
        }

        presentModes.clear();
        uint32_t modeCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, m_renderer.getSurface(), &modeCount, nullptr));
        presentModes.resize(modeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, m_renderer.getSurface(), &modeCount, presentModes.data()));

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
                    device, i, m_renderer.getSurface(), &presentSupport);
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

    auto checkFeatureSupport = [](const VkPhysicalDevice &device) {
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);
        return features.geometryShader && features.tessellationShader &&
               features.samplerAnisotropy;
    };

    auto evaluateDevice = [&deviceCount, &devices, &checkExtensionSupport,
                           &checkQueueSupport, &checkFeatureSupport,
                           &graphicsFamily, &presentFamily, &presentModes,
                           &surfaceFormats, &surfaceCapabilities, this]() {
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

        m_device = devices[deviceNum];
        m_surfCap = surfaceCapabilities;
        m_graphicsFI = graphicsFamily;
        m_presentFI = presentFamily;

        // Cache these for later
        vkGetPhysicalDeviceMemoryProperties(m_device, &m_memProps);

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
    std::set<int> uniQueue = {m_graphicsFI, m_presentFI};

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
    deviceCi.enabledExtensionCount =
        static_cast<uint32_t>(m_requiredExtensions.size());
    deviceCi.ppEnabledExtensionNames = m_requiredExtensions.data();
    deviceCi.enabledLayerCount =
        static_cast<uint32_t>(m_renderer.getValidationLayers().size());
    deviceCi.ppEnabledLayerNames = m_renderer.getValidationLayers().data();

    VK_CHECK(vkCreateDevice(m_device, &deviceCi, m_renderer.getAllocator(),
                            &m_handle));

    vkGetDeviceQueue(m_handle, m_graphicsFI, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_handle, m_presentFI, 0, &m_presentQueue);

    VkCommandPoolCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = m_graphicsFI;
    cpci.flags = 0;

    LOG("=Create command pool=");
    VK_CHECK(vkCreateCommandPool(m_handle, &cpci, m_renderer.getAllocator(),
                                 &m_commandPool));
}

void Device::destroy() {
    LOG("=Destroy command pool=");
    vkDestroyCommandPool(m_handle, m_commandPool, m_renderer.getAllocator());
    m_commandPool = VK_NULL_HANDLE;

    LOG("=Destroy logical device=");
    vkDestroyDevice(m_handle, m_renderer.getAllocator());
    m_handle = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_presentQueue = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
}

Logger &Device::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
