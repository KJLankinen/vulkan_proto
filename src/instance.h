#pragma once

#include "headers.h"

namespace vulkan_proto {
struct Instance{
    VulkanContext *m_ctx = nullptr;
    VkInstance m_handle = VK_NULL_HANDLE;

    VkDebugUtilsMessengerEXT m_dbgMsgr = VK_NULL_HANDLE;
    std::array<const char *, 1> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

    Instance();
    ~Instance();
    void create(VulkanContext *ctx);
    void destroy();
};
} // namespace vulkan_proto
