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

void init(Params &params);
void initGLFW(Params &params);
void createInstance(Params &params);
void createDevice(Params &params);
void createRenderPass(Params &params);
void createSwapchain(Params &params);
void pickSwapchainFormats(Params &params);
void createImage(Params &params, uint32_t width, uint32_t height,
                 uint32_t depth, VkFormat format, VkImageTiling tiling,
                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory);
uint32_t findMemoryType(Params &params, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);
void transitionImageLayout(Params &params, VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout);
VkCommandBuffer beginSingleTimeCommands(Params &params);
void endSingleTimeCommands(Params &params, VkCommandBuffer commandBuffer);
void abortIf(Params &params, bool condition, const char *msg,
             const char *fileName, int line);
void log(Params &params, Verbosity verbosity, FILE *stream, const char *msg);
void run();
void terminate(Params &params);
void terminateGLFW(Params &params);
} // namespace vulkan_proto
