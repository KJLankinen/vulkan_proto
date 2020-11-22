#pragma once
#include "device.h"
#include "headers.h"
#include "instance.h"
#include "render_pass.h"
#include "surface.h"
#include "swapchain.h"

namespace vulkan_proto {
VulkanContext m_ctx = {};
Instance m_instance = {};
Device m_device = {};
Surface m_surface = {};
Swapchain m_swapchain = {};
RenderPass m_renderPass = {};

VkSampler m_textureSampler = VK_NULL_HANDLE;
VkSemaphore m_imageAvailable = VK_NULL_HANDLE;
VkSemaphore m_renderingFinished = VK_NULL_HANDLE;

Renderer();
~Renderer();
void createTexturSampler();
void destroyTexturSampler();
void createSemaphores();
void destroySemaphores();
void init();
void terminate();
void run();
} // namespace vulkan_proto
