#include "swapchain.h"
#include "device.h"
#include "render_pass.h"
#include "renderer.h"

namespace vulkan_proto {
Swapchain::Swapchain(Renderer &renderer) : m_renderer(renderer) {}
Swapchain::~Swapchain() {}

void Swapchain::create(bool recycle) {
    LOG("=Create swap chain=");
    uint32_t modeCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_renderer.getPhysicalDevice(), m_renderer.getSurface(), &modeCount,
        nullptr));
    std::vector<VkPresentModeKHR> presentModes(modeCount);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_renderer.getPhysicalDevice(), m_renderer.getSurface(), &modeCount,
        presentModes.data()));

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto &availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }

    const VkSurfaceCapabilitiesKHR &surfCap =
        m_renderer.getSurfaceCapabilities();
    if (surfCap.currentExtent.width != 0xFFFFFFFF &&
        surfCap.currentExtent.height != 0xFFFFFFFF) {
        m_extent = surfCap.currentExtent;
    } else {
        VkExtent2D actualExtent = m_renderer.getWindowExtent();

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
    swapchainCi.surface = m_renderer.getSurface();
    swapchainCi.minImageCount = imageCount;
    swapchainCi.imageFormat = m_surfaceFormat.format;
    swapchainCi.imageColorSpace = m_surfaceFormat.colorSpace;
    swapchainCi.imageExtent = m_extent;
    swapchainCi.imageArrayLayers = 1;
    swapchainCi.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (m_renderer.getGraphicsFamilyIndex() !=
        m_renderer.getPresentFamilyIndex()) {
        const uint32_t queueFamilyIndices[] = {
            m_renderer.getGraphicsFamilyIndex(),
            m_renderer.getPresentFamilyIndex()};

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
    VK_CHECK(vkCreateSwapchainKHR(m_renderer.getDevice(), &swapchainCi,
                                  m_renderer.getAllocator(), &newChain));
    m_handle = newChain;

    if (recycle) {
        destroy(oldChain);
    }

    // Get images
    imageCount = 0;
    vkGetSwapchainImagesKHR(m_renderer.getDevice(), m_handle, &imageCount,
                            nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_renderer.getDevice(), m_handle, &imageCount,
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
        VK_CHECK(vkCreateImageView(m_renderer.getDevice(), &imageViewCi,
                                   m_renderer.getAllocator(), &m_views[i]));
    }

    // Depth
    m_renderer.createImage(
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

    VK_CHECK(vkCreateImageView(m_renderer.getDevice(), &imageViewCi,
                               m_renderer.getAllocator(), &m_depthView));

    // Transition depth image to correct layout
    VkCommandBuffer commandBuffer = m_renderer.beginSingleTimeCommands();
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_depthImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    if (m_depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        m_depthFormat == VK_FORMAT_D24_UNORM_S8_UINT) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
    m_renderer.endSingleTimeCommands(commandBuffer);

    // Framebuffers
    VkFramebufferCreateInfo framebufferCi = {};
    framebufferCi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCi.renderPass = m_renderer.getRenderPass();
    framebufferCi.width = m_extent.width;
    framebufferCi.height = m_extent.height;
    framebufferCi.layers = 1;
    framebufferCi.attachmentCount = 2;

    m_framebuffers.resize(m_images.size());
    for (size_t i = 0; i < m_framebuffers.size(); i++) {
        const VkImageView attachments[] = {m_views[i], m_depthView};
        framebufferCi.pAttachments = attachments;

        VK_CHECK(vkCreateFramebuffer(m_renderer.getDevice(), &framebufferCi,
                                     m_renderer.getAllocator(),
                                     &m_framebuffers[i]));
    }
}

void Swapchain::destroy(VkSwapchainKHR chain) {
    LOG("=Destroy swap chain=");
    vkDestroyImageView(m_renderer.getDevice(), m_depthView,
                       m_renderer.getAllocator());
    m_depthView = VK_NULL_HANDLE;

    vkDestroyImage(m_renderer.getDevice(), m_depthImage,
                   m_renderer.getAllocator());
    m_depthImage = VK_NULL_HANDLE;

    vkFreeMemory(m_renderer.getDevice(), m_depthMemory,
                 m_renderer.getAllocator());
    m_depthMemory = VK_NULL_HANDLE;

    for (auto &iv : m_views) {
        vkDestroyImageView(m_renderer.getDevice(), iv,
                           m_renderer.getAllocator());
    }
    m_views.clear();

    for (auto &fb : m_framebuffers) {
        vkDestroyFramebuffer(m_renderer.getDevice(), fb,
                             m_renderer.getAllocator());
    }
    m_framebuffers.clear();

    vkDestroySwapchainKHR(m_renderer.getDevice(), chain,
                          m_renderer.getAllocator());
}

void Swapchain::chooseFormats() {
    LOG("=Choose swapchain formats=");
    uint32_t formatCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_renderer.getPhysicalDevice(), m_renderer.getSurface(), &formatCount,
        nullptr));
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_renderer.getPhysicalDevice(), m_renderer.getSurface(), &formatCount,
        surfaceFormats.data()));

    if (surfaceFormats.size() == 1 &&
        surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
        m_surfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM,
                           VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    else if (surfaceFormats.size() > 1) {
        for (const auto &availableFormat : surfaceFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace ==
                    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                m_surfaceFormat = availableFormat;
                break;
            }
        }
    } else {
        m_surfaceFormat = surfaceFormats[0];
    }

    const std::array<VkFormat, 3> candidates = {VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                VK_FORMAT_D24_UNORM_S8_UINT};
    const VkFormatFeatureFlags features =
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (auto &format : candidates) {
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(m_renderer.getPhysicalDevice(),
                                            format, &props);

        if ((props.optimalTilingFeatures & features) == features) {
            m_depthFormat = format;
            break;
        }
    }

    THROW_IF(m_depthFormat == VK_FORMAT_UNDEFINED,
             "Failed to find a supported format!");
}

Logger &Swapchain::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
