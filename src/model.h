#pragma once

#include "headers.h"
#include "mesh.h"
#include "texture.h"

namespace vulkan_proto {

struct Renderer;
struct Logger;

struct Model {
    const Renderer &m_renderer;

    Mesh m_mesh;
    std::vector<Texture> m_textures;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    Model(Renderer &renderer);
    ~Model();
    void create(const char *root, const nlohmann::json &obj);
    void destroy();
    Logger &getLogger();
};
} // namespace vulkan_proto
