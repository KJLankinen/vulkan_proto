#pragma once
#include <stdint.h>

struct VkAllocationCallbacks;
struct GLFWwindow;

namespace vulkan_proto {

struct Instance;
struct Device;
struct Surface;
struct Swapchain;
struct RenderPass;

struct VulkanContext_Temp {
    Instance *instance = nullptr;
    Device *device = nullptr;
    Surface *surface = nullptr;
    Swapchain *swapchain = nullptr;
    RenderPass *renderPass = nullptr;

    VkAllocationCallbacks *allocator = nullptr;
    GLFWwindow *window = nullptr;
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
};
} // namespace vulkan_proto
