#include "swapchain.h"
#include "device.h"
#include "surface.h"
//#include "renderpass.h"

namespace vulkan_proto {
Swapchain::Swapchain(VulkanContext_Temp *ctx) : m_ctx(ctx) {}
Swapchain::~Swapchain() {}

void Swapchain::create(bool recycle) {
    LOG("=Create swap chain=");

    uint32_t modeCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx->device->m_device,
                                                       m_ctx->surface->m_handle,
                                                       &modeCount, nullptr));
    std::vector<VkPresentModeKHR> presentModes(modeCount);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_ctx->device->m_device, m_ctx->surface->m_handle, &modeCount,
        presentModes.data()));

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto &availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }

    const VkSurfaceCapabilitiesKHR &surfCap = m_ctx->device->m_surfCap;
    if (surfCap.currentExtent.width != 0xFFFFFFFF &&
        surfCap.currentExtent.height != 0xFFFFFFFF) {
        m_extent = surfCap.currentExtent;
    } else {
        VkExtent2D actualExtent = {m_ctx->windowWidth, m_ctx->windowHeight};

        actualExtent.width = std::max(
            surfCap.minImageExtent.width,
            std::min(surfCap.maxImageExtent.width, actualExtent.width));

        actualExtent.height = std::max(
            surfCap.minImageExtent.height,
            std::min(surfCap.maxImageExtent.height, actualExtent.height));

        m_extent = actualExtent;
    }

    uint32_t imageCount = surfCap.minImageCount + 1;
    if (surfCap.maxImageCount > 0 && imageCount > surfCap.maxImageCount) {
        imageCount = surfCap.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCi = {};
    swapchainCi.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCi.surface = m_ctx->surface->m_handle;
    swapchainCi.minImageCount = imageCount;
    swapchainCi.imageFormat = m_surfaceFormat.format;
    swapchainCi.imageColorSpace = m_surfaceFormat.colorSpace;
    swapchainCi.imageExtent = m_extent;
    swapchainCi.imageArrayLayers = 1;
    swapchainCi.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (m_ctx->device->m_graphicsFI != m_ctx->device->m_presentFI) {
        const uint32_t queueFamilyIndices[] = {
            (uint32_t)m_ctx->device->m_graphicsFI,
            (uint32_t)m_ctx->device->m_presentFI};

        swapchainCi.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCi.queueFamilyIndexCount = 2;
        swapchainCi.pQueueFamilyIndices = queueFamilyIndices;
    } else
        swapchainCi.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    swapchainCi.preTransform = surfCap.currentTransform;
    swapchainCi.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCi.presentMode = presentMode;
    swapchainCi.clipped = VK_TRUE;

    VkSwapchainKHR oldChain = m_handle;
    swapchainCi.oldSwapchain = oldChain;

    VkSwapchainKHR newChain = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSwapchainKHR(m_ctx->device->m_handle, &swapchainCi,
                                  m_ctx->allocator, &newChain));
    m_handle = newChain;

    if (recycle) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_images.size()); ++i) {
            vkDestroyImageView(m_ctx->device->m_handle, m_views[i],
                               m_ctx->allocator);
            vkDestroyFramebuffer(m_ctx->device->m_handle, m_framebuffers[i],
                                 m_ctx->allocator);
        }
        m_images.clear();
        m_views.clear();
        m_framebuffers.clear();

        vkDestroySwapchainKHR(m_ctx->device->m_handle, oldChain,
                              m_ctx->allocator);

        vkDestroyImageView(m_ctx->device->m_handle, m_depthView,
                           m_ctx->allocator);
        m_depthView = VK_NULL_HANDLE;

        vkDestroyImage(m_ctx->device->m_handle, m_depthImage, m_ctx->allocator);
        m_depthImage = VK_NULL_HANDLE;

        vkFreeMemory(m_ctx->device->m_handle, m_depthMemory, m_ctx->allocator);
        m_depthMemory = VK_NULL_HANDLE;
    }

    // Get images
    imageCount = 0;
    vkGetSwapchainImagesKHR(m_ctx->device->m_handle, m_handle, &imageCount,
                            nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_ctx->device->m_handle, m_handle, &imageCount,
                            m_images.data());

    // Create image views
    m_views.resize(imageCount);

    VkImageViewCreateInfo imageViewCi = {};
    imageViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCi.format = m_surfaceFormat.format;
    imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCi.subresourceRange.baseMipLevel = 0;
    imageViewCi.subresourceRange.levelCount = 1;
    imageViewCi.subresourceRange.baseArrayLayer = 0;
    imageViewCi.subresourceRange.layerCount = 1;

    for (uint32_t i = 0; i < imageCount; ++i) {
        imageViewCi.image = m_images[i];
        VK_CHECK(vkCreateImageView(m_ctx->device->m_handle, &imageViewCi,
                                   m_ctx->allocator, &m_views[i]));
    }

    // Depth
    m_ctx->device->createImage(
        m_extent.width, m_extent.height, 1, m_depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthMemory);

    imageViewCi = {};
    imageViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCi.image = m_depthImage;
    imageViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCi.format = m_depthFormat;
    imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewCi.subresourceRange.baseMipLevel = 0;
    imageViewCi.subresourceRange.levelCount = 1;
    imageViewCi.subresourceRange.baseArrayLayer = 0;
    imageViewCi.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_ctx->device->m_handle, &imageViewCi,
                               m_ctx->allocator, &m_depthView));

    m_ctx->device->transitionImageLayout(
        m_depthImage, m_depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    // Framebuffers
    VkFramebufferCreateInfo framebufferCi = {};
    framebufferCi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCi.renderPass = VK_NULL_HANDLE; // m_ctx->renderPass->m_handle;
    framebufferCi.width = m_extent.width;
    framebufferCi.height = m_extent.height;
    framebufferCi.layers = 1;
    framebufferCi.attachmentCount = 2;

    m_framebuffers.resize(m_images.size());
    for (size_t i = 0; i < m_framebuffers.size(); i++) {
        const VkImageView attachments[] = {m_views[i], m_depthView};
        framebufferCi.pAttachments = attachments;

        VK_CHECK(vkCreateFramebuffer(m_ctx->device->m_handle, &framebufferCi,
                                     m_ctx->allocator, &m_framebuffers[i]));
    }

    m_ctx->swapchain = this;
}

void Swapchain::destroy() {
    LOG("=Destroy swap chain=");
    vkDestroyImageView(m_ctx->device->m_handle, m_depthView, m_ctx->allocator);
    m_depthView = VK_NULL_HANDLE;

    vkDestroyImage(m_ctx->device->m_handle, m_depthImage, m_ctx->allocator);
    m_depthImage = VK_NULL_HANDLE;

    vkFreeMemory(m_ctx->device->m_handle, m_depthMemory, m_ctx->allocator);
    m_depthMemory = VK_NULL_HANDLE;

    for (auto &iv : m_views) {
        vkDestroyImageView(m_ctx->device->m_handle, iv, m_ctx->allocator);
    }
    m_views.clear();

    for (auto &fb : m_framebuffers) {
        vkDestroyFramebuffer(m_ctx->device->m_handle, fb, m_ctx->allocator);
    }
    m_framebuffers.clear();

    vkDestroySwapchainKHR(m_ctx->device->m_handle, m_handle, m_ctx->allocator);
    m_ctx->swapchain = nullptr;
}
} // namespace vulkan_proto
