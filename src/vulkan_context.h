#pragma once

#include <vulkan/vulkan.h>
#include "instance.h"

struct VulkanContext_Temp {
    Instance *instance = nullptr;
    VkAllocationCallbacks *allocator = nullptr;
};
