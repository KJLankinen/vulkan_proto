#pragma once

struct VkAllocationCallbacks;

namespace vulkan_proto {
struct Instance;
struct Device;
struct Surface;
struct Swapchain;
struct RenderPass;

struct VulkanContext {
    Instance *instance = nullptr;
    Device *device = nullptr;
    Surface *surface = nullptr;
    Swapchain *swapchain = nullptr;
    RenderPass *renderPass = nullptr;

    VkAllocationCallbacks *allocator = nullptr;
};
} // namespace vulkan_proto
