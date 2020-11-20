#pragma once
#include "data_types.h"
#include "util.h"
#include <array>
#include <assert.h>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

namespace vulkan_proto {

void init(VulkanContext &vkc, Log &log, GLFWwindow **window, uint32_t ww,
          uint32_t wh);
void initGLFW(GLFWwindow **window, uint32_t ww, uint32_t wh, Log &log);
void createInstance(VulkanContext &vkc, Log &log);
void createDevice(VulkanContext &vkc, Log &log);
void createRenderPass(VulkanContext &vkc, Log &log, bool recycle);
void createSwapchain(VulkanContext &vkc, Log &log, uint32_t ww, uint32_t wh,
                     bool recycle);
void pickSwapchainFormats(VulkanContext &vkc, Log &log);
void createImage(VulkanContext &vkc, Log &log, uint32_t width, uint32_t height,
                 uint32_t depth, VkFormat format, VkImageTiling tiling,
                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory);
uint32_t findMemoryType(VulkanContext &vkc, Log &log, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);
void transitionImageLayout(VulkanContext &vkc, Log &log, VkImage image,
                           VkFormat format, VkImageLayout oldLayout,
                           VkImageLayout newLayout);
VkCommandBuffer beginSingleTimeCommands(VulkanContext &vkc, Log &log);
void endSingleTimeCommands(VulkanContext &vkc, Log &log,
                           VkCommandBuffer commandBuffer);
void run();
void terminate(VulkanContext &vkc, Log &log, GLFWwindow *window);
void terminateGLFW(GLFWwindow *window, Log &log);
} // namespace vulkan_proto
