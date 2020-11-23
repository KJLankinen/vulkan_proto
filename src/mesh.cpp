#include "mesh.h"
#include "renderer.h"
#include <glm/gtx/hash.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace std {
template <> struct hash<vulkan_proto::Mesh::Vertex> {
    size_t operator()(vulkan_proto::Mesh::Vertex const &vertex) const {
        return ((hash<glm::vec3>()(vertex.position) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
} // namespace std

namespace vulkan_proto {
Mesh::Mesh(Renderer &renderer) : m_renderer(renderer) {}
Mesh::~Mesh() {}

void Mesh::create(const char *filename) {
    LOG("=Create mesh=");
    std::filesystem::path f{filename};
    THROW_IF(!std::filesystem::exists(f), "File %s does not exist", filename);

    auto moveData =
        [this](std::vector<auto> &data, VkBuffer &buffer,
               VkDeviceMemory &memory, VkBufferUsageFlags usage) {
            VkDeviceSize bufferSize = sizeof(data[0]) * data.size();
            m_renderer.createBuffer(bufferSize, usage,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer,
                                    memory);

            VkBuffer stagingBuffer = VK_NULL_HANDLE;
            VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
            m_renderer.createBuffer(bufferSize,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    stagingBuffer, stagingMemory);

            m_renderer.copyCPUToGPU(reinterpret_cast<const void *>(data.data()),
                                    bufferSize, stagingMemory, stagingBuffer,
                                    buffer);

            vkFreeMemory(m_renderer.getDevice(), stagingMemory,
                         m_renderer.getAllocator());
            vkDestroyBuffer(m_renderer.getDevice(), stagingBuffer,
                            m_renderer.getAllocator());
        };

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    std::string warn;
    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    THROW_IF(
        !tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename),
        "Failed to load model %s", filename);

    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            Vertex vertex = {};
            vertex.position = {attrib.vertices[3 * index.vertex_index + 0],
                               attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] =
                    static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }

            m_indices.push_back(uniqueVertices[vertex]);
        }
    }

    moveData(m_vertices, m_vertexBuffer, m_vertexMemory,
             VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    moveData(m_indices, m_indexBuffer, m_indexMemory,
             VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void Mesh::destroy() {
    LOG("=Destroy mesh=");
    vkDestroyBuffer(m_renderer.getDevice(), m_indexBuffer,
                    m_renderer.getAllocator());
    vkFreeMemory(m_renderer.getDevice(), m_indexMemory,
                 m_renderer.getAllocator());

    vkDestroyBuffer(m_renderer.getDevice(), m_vertexBuffer,
                    m_renderer.getAllocator());
    vkFreeMemory(m_renderer.getDevice(), m_vertexMemory,
                 m_renderer.getAllocator());

    m_vertexBuffer = VK_NULL_HANDLE;
    m_indexBuffer = VK_NULL_HANDLE;
    m_vertexMemory = VK_NULL_HANDLE;
    m_indexMemory = VK_NULL_HANDLE;
    m_vertices.clear();
    m_indices.clear();
}

Logger &Mesh::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
