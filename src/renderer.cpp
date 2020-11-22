#include "renderer.h"

namespace vulkan_proto {
Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::createTextureSampler() {
    LOG("=Create texture sampler=");
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy = 16;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 0.0f;

    VK_CHECK(vkCreateSampler(m_ctx.device->m_handle, &info, m_ctx.allocator,
                             &m_textureSampler));
}

void Renderer::destroyTextureSampler() {
    LOG("=Destroy texture sampler=");
    vkDestroySampler(m_ctx.device->m_handle, m_textureSampler, m_ctx.allocator);
}

void Renderer::createSemaphores() {
    LOG("=Create semaphores=");
    VkSemaphoreCreateInfo semaphoreCi = {};
    semaphoreCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK(vkCreateSemaphore(m_ctx.device->m_handle, &semaphoreCi,
                               m_ctx.allocator, &m_imageAvailable));
    VK_CHECK(vkCreateSemaphore(m_ctx.device->m_handle, &semaphoreCi,
                               m_ctx.allocator, &m_renderingFinished));
}

void Renderer::destroySemaphores() {
    LOG("=Destroy semaphores=");
    vkDestroySemaphore(m_ctx.device->m_handle, m_renderingFinished,
                       m_ctx.allocator);
    vkDestroySemaphore(m_ctx.device->m_handle, m_imageAvailable,
                       m_ctx.allocator);
}

void Renderer::init() {
    m_ctx.instance = &m_instance;
    m_ctx.device = &m_device;
    m_ctx.surface = &m_surface;
    m_ctx.swapchain = &m_swapchain;
    m_ctx.renderPass = &m_renderPass;

    m_surface.initWindow();
    m_instance.create(&m_ctx);
    m_surface.create(&m_ctx);
    m_device.create(&m_ctx);
    Swapchain::chooseFormats(&m_ctx, m_swapchain);
    m_renderPass.create(&m_ctx);
    m_swapchain.create(&m_ctx);
    createTextureSampler();
    createSemaphores();
}

void Renderer::terminate() {
    if (m_device.m_handle != VK_NULL_HANDLE) {
        VK_CHECK(vkDeviceWaitIdle(m_device.m_handle));
    }

    destroySemaphores();
    destroyTextureSampler();
    m_swapchain.destroy(m_swapchain.m_handle);
    m_renderPass.destroy();
    m_device.destroy();
    m_surface.destroy();
    m_instance.destroy();
    m_surface.terminateWindow();
    // flush logger
}

void Renderer::run() {
    try {
        init();
    } catch (const std::runtime_error &e) {
        printf("Unhandled exception: %s\n", e.what());
    }

    terminate();
}
} // namespace vulkan_proto
