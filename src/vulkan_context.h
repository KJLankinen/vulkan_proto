#pragma once

struct VkAllocationCallbacks;
struct GLFWwindow;

namespace vulkan_proto {

struct Instance;
struct Device;
struct Surface;

struct VulkanContext_Temp {
    Instance *instance = nullptr;
    Device *device = nullptr;
    Surface *surface = nullptr;
    VkAllocationCallbacks *allocator = nullptr;
    GLFWwindow *window = nullptr;
};
} // namespace vulkan_proto
