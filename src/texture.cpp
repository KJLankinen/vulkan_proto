#include "texture.h"
#include "renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace vulkan_proto {
Texture::Texture(const Renderer &renderer) : m_renderer(renderer) {}
Texture::~Texture() {}

void Texture::create(const char *filename) {
    LOG("=Create texture=");

    std::filesystem::path f{filename};
    THROW_IF(!std::filesystem::exists(f), "File %s does not exist", filename);

    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc *pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels,
                                STBI_rgb_alpha);
    THROW_IF(pixels == nullptr, "Failed to load texture image!");

    VkDeviceSize imageSize = texWidth * texHeight * 4;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    m_renderer.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingBuffer, stagingBufferMemory);

    void *data = nullptr;
    vkMapMemory(m_renderer.getDevice(), stagingBufferMemory, 0, imageSize, 0,
                &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_renderer.getDevice(), stagingBufferMemory);

    stbi_image_free(pixels);

    m_renderer.createImage(
        texWidth, texHeight, 1, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_memory);

    m_renderer.transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_UNORM,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_renderer.copyBufferToImage(stagingBuffer, m_image,
                                 static_cast<uint32_t>(texWidth),
                                 static_cast<uint32_t>(texHeight));
    m_renderer.transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_UNORM,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkFreeMemory(m_renderer.getDevice(), stagingBufferMemory,
                 m_renderer.getAllocator());
    vkDestroyBuffer(m_renderer.getDevice(), stagingBuffer,
                    m_renderer.getAllocator());

    VkImageViewCreateInfo imageViewCI = {};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.image = m_image;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_renderer.getDevice(), &imageViewCI,
                               m_renderer.getAllocator(), &m_view));

    // Create image info
    m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    m_descriptor.imageLayout = m_layout;
    m_descriptor.imageView = m_view;
}

void Texture::destroy() {
    LOG("=Destroy texture=");
    vkDestroyImageView(m_renderer.getDevice(), m_view,
                       m_renderer.getAllocator());
    vkDestroyImage(m_renderer.getDevice(), m_image, m_renderer.getAllocator());
    vkFreeMemory(m_renderer.getDevice(), m_memory, m_renderer.getAllocator());

    m_view = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
}

Logger &Texture::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
