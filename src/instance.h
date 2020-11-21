#pragma once

#include "headers.h"
#include "vulkan_context.h"
#include <array>
#include <iostream>

namespace vulkan_proto {
struct Instance{
    VkInstance m_handle = VK_NULL_HANDLE;
    VulkanContext_Temp *m_ctx = nullptr;

    VkDebugUtilsMessengerEXT m_dbgMsgr = VK_NULL_HANDLE;
    std::array<const char *, 1> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

    Instance(VulkanContext_Temp *ctx);
    ~Intance() {}
    void create();
    void destroy();
};
} // namespace vulkan_proto
