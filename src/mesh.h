#pragma once

#include "headers.h"

namespace vulkan_proto {

struct Renderer;
struct Logger;

struct Mesh {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 texCoord;

        bool operator==(const Vertex &o) const {
            return position == o.position && color == o.color &&
                   texCoord == o.texCoord;
        }
    };

    const Renderer &m_renderer;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
    VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    Mesh(Renderer &renderer);
    ~Mesh();
    void create(const char *filename);
    void destroy();
    Logger &getLogger();
};
} // namespace vulkan_proto
