#pragma once
#include "device.h"
#include "headers.h"
#include "instance.h"
#include "render_pass.h"
#include "surface.h"
#include "swapchain.h"

namespace vulkan_proto {
struct Renderer {
  private:
    Instance m_instance;
    Device m_device;
    Surface m_surface;
    Swapchain m_swapchain;
    RenderPass m_renderPass;

    VkAllocationCallbacks *m_allocator = nullptr;

    VkSampler m_textureSampler = VK_NULL_HANDLE;
    VkSemaphore m_imageAvailable = VK_NULL_HANDLE;
    VkSemaphore m_renderingFinished = VK_NULL_HANDLE;

  public:
    Renderer();
    ~Renderer();
    void run();

    const VkInstance &getInstance() const { return m_instance.m_handle; }
    const VkDevice &getDevice() const { return m_device.m_handle; }
    const VkPhysicalDevice &getPhysicalDevice() const {
        return m_device.m_device;
    }
    const VkSurfaceKHR &getSurface() const { return m_surface.m_handle; }
    const VkSwapchainKHR &getSwapchain() const { return m_swapchain.m_handle; }
    const VkRenderPass &getRenderPass() const { return m_renderPass.m_handle; }
    const VkAllocationCallbacks *getAllocator() const { return m_allocator; }
    const std::array<const char *, 1> &getValidationLayers() const {
        return m_instance.m_validationLayers;
    }
    const VkFormat &getSurfaceFormat() const {
        return m_swapchain.m_surfaceFormat.format;
    }
    const VkFormat &getDepthFormat() const { return m_swapchain.m_depthFormat; }
    VkExtent2D getWindowExtent() const {
        return VkExtent2D{m_surface.m_windowWidth, m_surface.m_windowHeight};
    }
    const VkSurfaceCapabilitiesKHR &getSurfaceCapabilities() const {
        return m_device.m_surfCap;
    }
    uint32_t getGraphicsFamilyIndex() const {
        return (uint32_t)m_device.m_graphicsFI;
    }
    uint32_t getPresentFamilyIndex() const {
        return (uint32_t)m_device.m_presentFI;
    }

    void createImage(uint32_t width, uint32_t height, uint32_t depth,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage &image, VkDeviceMemory &imageMemory) const;
    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout) const;
    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties) const;
    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

  private:
    void createTextureSampler();
    void destroyTextureSampler();
    void createSemaphores();
    void destroySemaphores();
    void init();
    void terminate();
};
} // namespace vulkan_proto
