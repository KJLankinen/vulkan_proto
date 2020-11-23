#include "model.h"
#include "renderer.h"

namespace vulkan_proto {
Model::Model(Renderer &renderer) : m_renderer(renderer), m_mesh(renderer) {}
Model::~Model() {}

void Model::create(const char *root, const nlohmann::json &obj) {
    LOG("=Create model=");
    std::string meshPath(root + obj.at("mesh").get<std::string>());
    m_mesh.create(meshPath.c_str());

    for (const auto &it : obj.at("textures")) {
        std::string texturePath(root + it.get<std::string>());
        m_textures.push_back(m_renderer);
        m_textures.back().create(texturePath.c_str());
    }

    VkDeviceSize bufferSize = sizeof(m_modelMatrix);
    m_renderer.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            m_uniformBuffer.stagingBuffer,
                            m_uniformBuffer.stagingMemory);

    m_renderer.createBuffer(bufferSize,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            m_uniformBuffer.buffer, m_uniformBuffer.memory);

    m_modelMatrix *= glm::scale(
        glm::mat4(1.0f), glm::vec3(obj.at("scale").at("x").get<float>(),
                                   obj.at("scale").at("y").get<float>(),
                                   obj.at("scale").at("z").get<float>()));
    m_modelMatrix = glm::translate(
        m_modelMatrix, glm::vec3(obj.at("position").at("x").get<float>(),
                                 obj.at("position").at("y").get<float>(),
                                 obj.at("position").at("z").get<float>()));
}

void Model::destroy() {
    LOG("=Destroy model=");
    m_mesh.destroy();
    for (auto texture : m_textures) {
        texture.destroy();
    }
    m_descriptorSet = VK_NULL_HANDLE;
    vkDestroyBuffer(m_renderer.getDevice(), m_uniformBuffer.stagingBuffer,
                    m_renderer.getAllocator());
    vkDestroyBuffer(m_renderer.getDevice(), m_uniformBuffer.buffer,
                    m_renderer.getAllocator());
    vkFreeMemory(m_renderer.getDevice(), m_uniformBuffer.stagingMemory,
                 m_renderer.getAllocator());
    vkFreeMemory(m_renderer.getDevice(), m_uniformBuffer.memory,
                 m_renderer.getAllocator());
}

Logger &Model::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
