#pragma once

#include "headers.h"

namespace vulkan_proto {
struct Renderer;
struct Instance{
    const Renderer &m_renderer;

    VkInstance m_handle = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_dbgMsgr = VK_NULL_HANDLE;
    std::array<const char *, 1> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

    Instance(Renderer &renderer);
    ~Instance();
    void create();
    void destroy();
};
} // namespace vulkan_proto
