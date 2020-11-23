#include "renderer.h"

namespace vulkan_proto {
Renderer::Renderer()
    : m_instance(*this), m_device(*this), m_surface(*this), m_swapchain(*this),
      m_renderPass(*this), m_graphicsPipeline(*this), m_camera(*this),
      m_logger("vulkan_proto.log") {}
Renderer::~Renderer() {}

void Renderer::init() {
    m_surface.initWindow();
    m_instance.create();
    m_surface.create();
    m_device.create();
    m_swapchain.chooseFormats();
    m_renderPass.create();
    m_swapchain.create();
    createTextureSampler();
    createSemaphores();
    createModels();
    setupDescriptors();
    m_graphicsPipeline.create();
    recordCommandBuffers();
}

void Renderer::terminate() {
    if (m_device.m_handle != VK_NULL_HANDLE) {
        VK_CHECK(vkDeviceWaitIdle(m_device.m_handle));
    }
    m_graphicsPipeline.destroy();

    vkDestroyDescriptorPool(m_device.m_handle, m_descriptorPool, m_allocator);

    for (auto &dsl : m_descriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(m_device.m_handle, dsl, m_allocator);
    }

    for (auto &model : m_models) {
        model.destroy();
    }

    LOG("=Destroy semaphores=");
    vkDestroySemaphore(m_device.m_handle, m_renderingFinished, m_allocator);
    vkDestroySemaphore(m_device.m_handle, m_imageAvailable, m_allocator);

    LOG("=Destroy texture sampler=");
    vkDestroySampler(m_device.m_handle, m_textureSampler, m_allocator);

    m_swapchain.destroy(getSwapchain());
    m_renderPass.destroy();
    m_device.destroy();
    m_surface.destroy();
    m_instance.destroy();
    m_surface.terminateWindow();

    m_logger.flush();
}

void Renderer::run(const char *inputFileName) {
    std::ifstream file(inputFileName, std::ios::in);
    if (file.is_open() == false) {
        printf("Could not open the input file with name %s\n", inputFileName);
        return;
    }

    file >> m_programInput;
    file.close();
    m_dataPath = m_programInput.at("data_path").get<std::string>().c_str();

    try {
        init();
        loop();
    } catch (const std::runtime_error &e) {
        printf("Unhandled exception: %s\n", e.what());
    }

    terminate();
}

void Renderer::loop() {
    while (!glfwWindowShouldClose(m_surface.m_window)) {
        if (m_windowResized) {
            onWindowResize();
            m_windowResized = false;
        }
        render();
        glfwPollEvents();
    }
}

void Renderer::render() {
    updateUniformBuffers();
    drawFrame();
}

void Renderer::drawFrame() {
    uint32_t imageIndex = ~0U;
    VkResult result = vkAcquireNextImageKHR(
        m_device.m_handle, m_swapchain.m_handle, (uint64_t)10, m_imageAvailable,
        VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result == VK_NOT_READY) {
        return;
    }

    THROW_IF(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR,
             "Failed to acquire swap chain image, resulted in %d", result);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailable};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {m_renderingFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(m_device.m_graphicsQueue, 1, &submitInfo,
                           VK_NULL_HANDLE));

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain.m_handle;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_device.m_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else {
        VK_CHECK(result);
    }
}

void Renderer::onWindowResize() {
    if (m_surface.m_windowWidth > 0 && m_surface.m_windowHeight > 0) {
        if (m_device.m_handle != VK_NULL_HANDLE &&
            m_surface.m_handle != VK_NULL_HANDLE) {
            recreateSwapchain();
        }
    }
}

void Renderer::recreateSwapchain() {
    if (m_device.m_handle == VK_NULL_HANDLE) {
        return;
    }
    VK_CHECK(vkDeviceWaitIdle(m_device.m_handle));
    m_renderPass.create(true);
    m_swapchain.create(true);
    m_graphicsPipeline.create(true);
    recordCommandBuffers();
}

void Renderer::updateUniformBuffers() {
    float aspectRatio = m_swapchain.m_extent.width /
                        static_cast<float>(m_swapchain.m_extent.height);
    if (aspectRatio == 0.0f) {
        aspectRatio = 1.0f;
    }

    static const glm::mat4 vulkanClipFix =
        glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                  0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);

    glm::mat4 vp = vulkanClipFix;
    vp *= glm::perspective(glm::radians(m_camera.m_fov), aspectRatio,
                           m_camera.m_near, m_camera.m_far);
    vp *= m_camera.getLookAt();

    for (auto &model : m_models) {
        glm::mat4 modelMatrix = vp * model.m_modelMatrix;
        copyCPUToGPU(reinterpret_cast<const void *>(&modelMatrix),
                     (VkDeviceSize)sizeof(modelMatrix),
                     model.m_uniformBuffer.stagingMemory,
                     model.m_uniformBuffer.stagingBuffer,
                     model.m_uniformBuffer.buffer);
    }
}

void Renderer::recordCommandBuffers() {
    LOG("=Recording command buffers=");
    // If we have old command buffers, let's free them and create new ones.
    if (m_commandBuffers.size() > 0)
        vkFreeCommandBuffers(m_device.m_handle, m_device.m_commandPool,
                             static_cast<uint32_t>(m_commandBuffers.size()),
                             m_commandBuffers.data());

    m_commandBuffers.resize(m_swapchain.m_framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_device.m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

    VK_CHECK(vkAllocateCommandBuffers(m_device.m_handle, &allocInfo,
                                      m_commandBuffers.data()));

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_commandBuffers.size());
         ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        // Begin recording a command buffer
        VK_CHECK(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo));

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass.m_handle;
        renderPassInfo.framebuffer = m_swapchain.m_framebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapchain.m_extent;

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount =
            static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        VkDeviceSize offsets[] = {0};

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_graphicsPipeline.m_handle);
        // Common set
        vkCmdBindDescriptorSets(m_commandBuffers[i],
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_graphicsPipeline.m_layout, 0, 1,
                                &m_commonDescriptorSet, 0, nullptr);

        for (auto model : m_models) {
            vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1,
                                   &model.m_mesh.m_vertexBuffer, offsets);
            vkCmdBindIndexBuffer(m_commandBuffers[i],
                                 model.m_mesh.m_indexBuffer, 0,
                                 VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(m_commandBuffers[i],
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_graphicsPipeline.m_layout, 1, 1,
                                    &model.m_descriptorSet, 0, nullptr);
            vkCmdDrawIndexed(m_commandBuffers[i], model.m_mesh.m_indices.size(),
                             1, 0, 0, 0);
        }

        vkCmdEndRenderPass(m_commandBuffers[i]);

        VK_CHECK(vkEndCommandBuffer(m_commandBuffers[i]));
    }
}

void Renderer::setupDescriptors() {
    LOG("=Setup descriptors=");
    m_descriptorSetLayouts.clear();
    m_descriptorSetLayouts.resize(2);

    // Texture sampler bindings
    // layout (set = 0, binding = 0)
    std::array<VkDescriptorSetLayoutBinding, 1> commonBindings;
    commonBindings[0].binding = 0;
    commonBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    commonBindings[0].descriptorCount = 1;
    commonBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    commonBindings[0].pImmutableSamplers = &m_textureSampler;

    // Object bindings
    // First one for model matrix
    // layout (set = 1, binding = 0)
    std::vector<VkDescriptorSetLayoutBinding> objectBindings;
    objectBindings.emplace_back();
    objectBindings.back().binding = 0;
    objectBindings.back().descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // If an array, 'descriptorCount', should equal that
    objectBindings.back().descriptorCount = 1;
    objectBindings.back().stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    objectBindings.back().pImmutableSamplers = nullptr;

    // Second one for textures
    // layout (set = 1, binding = 1..N)
    // This is hacky, it assumes all models have the same textures size
    for (uint32_t i = 0; i < m_models[0].m_textures.size(); i++) {
        objectBindings.emplace_back();
        objectBindings.back().binding = i + 1;
        objectBindings.back().descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        objectBindings.back().descriptorCount = 1;
        objectBindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        objectBindings.back().pImmutableSamplers = nullptr;
    }

    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCis[] = {
        {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
            nullptr,                                             // pNext
            0,                                                   // flags
            static_cast<uint32_t>(commonBindings.size()),        // bindingCount
            commonBindings.data()                                // pBindings
        },
        {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
            nullptr,                                             // pNext
            0,                                                   // flags
            static_cast<uint32_t>(objectBindings.size()),        // bindingCount
            objectBindings.data()                                // pBindings
        },
    };

    VK_CHECK(vkCreateDescriptorSetLayout(
        m_device.m_handle, &descriptorSetLayoutCis[0], m_allocator,
        &m_descriptorSetLayouts[0]));

    VK_CHECK(vkCreateDescriptorSetLayout(
        m_device.m_handle, &descriptorSetLayoutCis[1], m_allocator,
        &m_descriptorSetLayouts[1]));

    std::array<VkDescriptorPoolSize, 3> descriptorPoolSizes;
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorPoolSizes[0].descriptorCount = 1;
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[1].descriptorCount =
        static_cast<uint32_t>(m_models.size());
    descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorPoolSizes[2].descriptorCount =
        static_cast<uint32_t>(m_models.size() * m_models[0].m_textures.size());

    VkDescriptorPoolCreateInfo descriptorPoolCI = {};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.poolSizeCount =
        static_cast<uint32_t>(descriptorPoolSizes.size());
    descriptorPoolCI.pPoolSizes = descriptorPoolSizes.data();
    // 1 for the sampler + 1 per each object
    descriptorPoolCI.maxSets = 1 + static_cast<uint32_t>(m_models.size());

    VK_CHECK(vkCreateDescriptorPool(m_device.m_handle, &descriptorPoolCI,
                                    m_allocator, &m_descriptorPool));

    // Common set
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayouts[0];

    VK_CHECK(vkAllocateDescriptorSets(m_device.m_handle, &allocInfo,
                                      &m_commonDescriptorSet));

    // Per object sets
    for (auto &model : m_models) {
        allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayouts[1];

        VK_CHECK(vkAllocateDescriptorSets(m_device.m_handle, &allocInfo,
                                          &model.m_descriptorSet));

        model.m_uniformBuffer.descriptor.buffer = model.m_uniformBuffer.buffer;
        model.m_uniformBuffer.descriptor.offset = 0;
        model.m_uniformBuffer.descriptor.range = sizeof(model.m_modelMatrix);

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.emplace_back();
        descriptorWrites.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.back().dstSet = model.m_descriptorSet;
        descriptorWrites.back().dstBinding = 0;
        descriptorWrites.back().dstArrayElement = 0;
        descriptorWrites.back().descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites.back().descriptorCount = 1;
        descriptorWrites.back().pBufferInfo = &model.m_uniformBuffer.descriptor;

        uint32_t dstBinding = 1;
        for (auto &texture : model.m_textures) {
            descriptorWrites.emplace_back();
            descriptorWrites.back().sType =
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites.back().dstSet = model.m_descriptorSet;
            descriptorWrites.back().dstBinding = dstBinding++;
            descriptorWrites.back().dstArrayElement = 0;
            descriptorWrites.back().descriptorType =
                VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites.back().descriptorCount = 1;
            descriptorWrites.back().pImageInfo = &texture.m_descriptor;
        }

        vkUpdateDescriptorSets(m_device.m_handle,
                               static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void Renderer::createModels() {
    std::string modelsPath(m_programInput.at("data_path").get<std::string>() +
                           m_programInput.at("models")
                               .at("path")
                               .get<std::string>());
    for (const auto &it : m_programInput.at("models").at("objs")) {
        m_models.push_back(Model(*this));
        m_models.back().create(modelsPath.c_str(), it);
    }
}

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

    VK_CHECK(vkCreateSampler(m_device.m_handle, &info, m_allocator,
                             &m_textureSampler));
}

void Renderer::createSemaphores() {
    LOG("=Create semaphores=");
    VkSemaphoreCreateInfo semaphoreCi = {};
    semaphoreCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK(vkCreateSemaphore(m_device.m_handle, &semaphoreCi, m_allocator,
                               &m_imageAvailable));
    VK_CHECK(vkCreateSemaphore(m_device.m_handle, &semaphoreCi, m_allocator,
                               &m_renderingFinished));
}

void Renderer::copyCPUToGPU(const void *srcData, VkDeviceSize sizeInBytes,
                            VkDeviceMemory stagingMemory,
                            VkBuffer stagingBuffer, VkBuffer dstBuffer) const {
    THROW_IF(srcData == nullptr, "Source data is nullptr");
    THROW_IF(sizeInBytes <= 0, "Size to copy is zero");
    THROW_IF(stagingMemory == VK_NULL_HANDLE, "Staging memory is null handle");
    THROW_IF(stagingBuffer == VK_NULL_HANDLE, "Staging buffer is null handle");
    THROW_IF(dstBuffer == VK_NULL_HANDLE, "Destination buffer is null handle");

    void *data = nullptr;
    vkMapMemory(m_device.m_handle, stagingMemory, 0, sizeInBytes, 0, &data);
    memcpy(data, srcData, sizeInBytes);
    vkUnmapMemory(m_device.m_handle, stagingMemory);
    copyBuffer(stagingBuffer, dstBuffer, sizeInBytes);
}

void Renderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                                 uint32_t height) const {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endSingleTimeCommands(commandBuffer);
}

void Renderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                          VkDeviceSize size) const {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    endSingleTimeCommands(commandBuffer);
}

void Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                            VkMemoryPropertyFlags properties, VkBuffer &buffer,
                            VkDeviceMemory &bufferMemory) const {
    VkBufferCreateInfo bufferCI = {};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = size;
    bufferCI.usage = usage;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(
        vkCreateBuffer(m_device.m_handle, &bufferCI, m_allocator, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device.m_handle, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits, properties);
    VK_CHECK(vkAllocateMemory(m_device.m_handle, &allocInfo, nullptr,
                              &bufferMemory));
    vkBindBufferMemory(m_device.m_handle, buffer, bufferMemory, 0);
}

void Renderer::createImage(uint32_t width, uint32_t height, uint32_t depth,
                           VkFormat format, VkImageTiling tiling,
                           VkImageUsageFlags usage,
                           VkMemoryPropertyFlags properties, VkImage &image,
                           VkDeviceMemory &imageMemory) const {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = depth;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    VK_CHECK(vkCreateImage(m_device.m_handle, &imageInfo, m_allocator, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device.m_handle, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHECK(
        vkAllocateMemory(m_device.m_handle, &allocInfo, nullptr, &imageMemory));

    VK_CHECK(vkBindImageMemory(m_device.m_handle, image, imageMemory, 0));
}

void Renderer::transitionImageLayout(VkImage image, VkFormat format,
                                     VkImageLayout oldLayout,
                                     VkImageLayout newLayout) const {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        THROW_IF(true, "Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(commandBuffer);
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter,
                                  VkMemoryPropertyFlags properties) const {
    for (uint32_t i = 0; i < m_device.m_memProps.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) &&
            (m_device.m_memProps.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    THROW_IF(true, "Failed to find suitable memory type!");

    return ~0u;
}

VkCommandBuffer Renderer::beginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_device.m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(m_device.m_handle, &allocInfo,
                                      &commandBuffer));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
    if (commandBuffer != VK_NULL_HANDLE) {
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VK_CHECK(vkQueueSubmit(m_device.m_graphicsQueue, 1, &submitInfo,
                               VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(m_device.m_graphicsQueue));

        vkFreeCommandBuffers(m_device.m_handle, m_device.m_commandPool, 1,
                             &commandBuffer);
    }
}
} // namespace vulkan_proto
