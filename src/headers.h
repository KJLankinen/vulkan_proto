#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include "vulkan_context.h"
#include <GLFW/glfw3.h>
#include <array>
#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>

#define VK_CHECK(call) call
#define THROW_IF(condition, ...)                                               \
    do {                                                                       \
    } while (0)
#define LOG(...)                                                               \
    do {                                                                       \
    } while (0)
