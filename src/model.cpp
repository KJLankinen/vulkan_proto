#include "model.h"
#include "renderer.h"

namespace vulkan_proto {
Model::Model(Renderer &renderer) : m_renderer(renderer), m_mesh(renderer) {}
Model::~Model() {}

void Model::create(const char *root, const nlohmann::json &obj) {
    std::string meshPath(root + obj.at("mesh").get<std::string>());
    m_mesh.create(meshPath.c_str());

    for (const auto &it : obj.at("textures")) {
        std::string texturePath(root + it.get<std::string>());
        m_textures.push_back(m_renderer);
        m_textures.back().create(texturePath.c_str());
    }
}

void Model::destroy() {
    m_mesh.destroy();
    for (auto texture : m_textures) {
        texture.destroy();
    }
    m_descriptorSet = VK_NULL_HANDLE;
}

Logger &Model::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
