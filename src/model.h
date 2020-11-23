#pragma once

#include "headers.h"
#include "mesh.h"
#include "texture.h"

namespace vulkan_proto {

struct Renderer;
struct Logger;

struct Model {
    const Renderer &m_renderer;

    // TODO these should be pointers, so that multiple instances can use the
    // same mesh and textures
    Mesh m_mesh;
    std::vector<Texture> m_textures;

    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);

    struct Buffer {
        VkDescriptorBufferInfo descriptor;
        VkBuffer stagingBuffer;
        VkBuffer buffer;
        VkDeviceMemory stagingMemory;
        VkDeviceMemory memory;
    } m_uniformBuffer;

    Model(Renderer &renderer);
    ~Model();
    void create(const char *root, const nlohmann::json &obj);
    void destroy();
    Logger &getLogger();
};
} // namespace vulkan_proto
