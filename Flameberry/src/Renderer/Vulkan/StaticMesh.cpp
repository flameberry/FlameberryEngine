#include "StaticMesh.h"

#include "Core/Core.h"
#include "Core/Timer.h"

#include "VulkanRenderCommand.h"
#include "Renderer/Material.h"
#include "AssetManager/AssetManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace Flameberry {
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    };
}

namespace std {
    template <>
    struct hash<Flameberry::VulkanVertex> {
        size_t operator()(Flameberry::VulkanVertex const& vertex) const {
            size_t seed = 0;
            Flameberry::hashCombine(seed, vertex.Position, vertex.Normal, vertex.TextureUV, vertex.Tangent, vertex.BiTangent);
            return seed;
        }
    };
}

namespace Flameberry {
    StaticMesh::StaticMesh(const std::string& path)
        : m_FilePath(path)
    {
        Load(path);
        CreateBuffers();

        uint32_t lengthSlash = m_FilePath.find_last_of('/') + 1;
        uint32_t lengthDot = m_FilePath.find_last_of('.');
        m_Name = m_FilePath.substr(lengthSlash, lengthDot - lengthSlash);

        FL_TRACE("Allocated {0}, {1} bytes for {2}: Vertices, Indices", m_Vertices.size() * sizeof(VulkanVertex), m_Indices.size() * sizeof(uint32_t), m_Name);
    }

    StaticMesh::StaticMesh(const std::vector<VulkanVertex>& vertices, const std::vector<uint32_t>& indices)
        : m_Vertices(vertices), m_Indices(indices)
    {
        CreateBuffers();
    }

    void StaticMesh::Bind(VkCommandBuffer commandBuffer) const
    {
        VkBuffer vk_vertex_buffers[] = { m_VertexBuffer->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vk_vertex_buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    void StaticMesh::OnDraw(VkCommandBuffer commandBuffer) const
    {
        vkCmdDrawIndexed(commandBuffer, (uint32_t)m_Indices.size(), 1, 0, 0, 0);
    }

    void StaticMesh::OnDrawSubMesh(VkCommandBuffer commandBuffer, uint32_t subMeshIndex) const
    {
        auto& submesh = m_SubMeshes[subMeshIndex];
        vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.IndexOffset, 0, 0);
    }

    void StaticMesh::Load(const std::string& path)
    {
        FL_SCOPED_TIMER("Load_Model");
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        size_t pos = path.find_last_of('/');
        std::string mtlBaseDir = path.substr(0, pos);
        FL_ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), mtlBaseDir.c_str()), err);

        bool has_tex_coord = attrib.texcoords.size();
        std::unordered_map<VulkanVertex, uint32_t> uniqueVertices{};

        std::vector<UUID> materialUUIDs;

        for (const auto& mat : materials) {
            auto materialAsset = AssetManager::CreateAsset<Material>();
            materialAsset->IsDerived = true;
            materialAsset->Name = mat.name;
            materialAsset->Albedo = { mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] };
            materialAsset->Roughness = mat.roughness;

            materialAsset->TextureMapEnabled = !mat.diffuse_texname.empty();
            if (materialAsset->TextureMapEnabled)
                materialAsset->TextureMap = VulkanTexture::TryGetOrLoadTexture(mtlBaseDir + "/" + mat.diffuse_texname);

            materialAsset->NormalMapEnabled = !mat.displacement_texname.empty();
            if (materialAsset->NormalMapEnabled)
                materialAsset->NormalMap = VulkanTexture::TryGetOrLoadTexture(mtlBaseDir + "/" + mat.displacement_texname);

            materialUUIDs.emplace_back(materialAsset->GetUUID());
        }

        std::vector<VulkanVertex> tempVertices;

        for (const auto& shape : shapes)
        {
            int i = 0;
            uint32_t indexCount = m_Indices.size();

            VulkanVertex triangleVertices[3] = {};

            for (const auto& index : shape.mesh.indices)
            {
                triangleVertices[i % 3].Position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                triangleVertices[i % 3].Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };

                if (has_tex_coord) {
                    triangleVertices[i % 3].TextureUV = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                i++;

                if (i % 3 == 0)
                {
                    glm::vec3 vertex0 = triangleVertices[0].Position;
                    glm::vec3 vertex1 = triangleVertices[1].Position;
                    glm::vec3 vertex2 = triangleVertices[2].Position;

                    glm::vec3 tangent, bitangent;

                    glm::vec3 edge1 = vertex1 - vertex0;
                    glm::vec3 edge2 = vertex2 - vertex0;

                    glm::vec2 uv0 = triangleVertices[0].TextureUV;
                    glm::vec2 uv1 = triangleVertices[1].TextureUV;
                    glm::vec2 uv2 = triangleVertices[2].TextureUV;

                    glm::vec2 deltaUV1 = uv1 - uv0;
                    glm::vec2 deltaUV2 = uv2 - uv0;

                    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

                    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
                    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
                    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

                    triangleVertices[0].Tangent = tangent;
                    triangleVertices[0].BiTangent = bitangent;

                    triangleVertices[1].Tangent = tangent;
                    triangleVertices[1].BiTangent = bitangent;

                    triangleVertices[2].Tangent = tangent;
                    triangleVertices[2].BiTangent = bitangent;

                    for (uint16_t j = 0; j < 3; j++)
                    {
                        auto& vertex = triangleVertices[j];

                        if (uniqueVertices.count(vertex) == 0) {
                            uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                            m_Vertices.push_back(vertex);
                        }
                        m_Indices.push_back(uniqueVertices[vertex]);
                    }
                }
            }

            auto& submesh = m_SubMeshes.emplace_back();
            submesh.IndexCount = m_Indices.size() - indexCount;
            submesh.IndexOffset = indexCount;

            if (shape.mesh.material_ids[0] != -1)
                submesh.MaterialUUID = materialUUIDs[shape.mesh.material_ids[0]];
        }
        FL_INFO("Loaded Model: '{0}': Vertices: {1}, Indices: {2}", path, m_Vertices.size(), m_Indices.size());
    }

    void StaticMesh::CreateBuffers()
    {
        {
            // Creating Vertex Buffer
            VkDeviceSize bufferSize = sizeof(VulkanVertex) * m_Vertices.size();
            VulkanBuffer stagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            stagingBuffer.MapMemory(bufferSize);
            stagingBuffer.WriteToBuffer(m_Vertices.data(), bufferSize, 0);
            stagingBuffer.UnmapMemory();

            m_VertexBuffer = std::make_unique<VulkanBuffer>(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VulkanRenderCommand::CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
        }

        {
            // Creating Index Buffer
            VkDeviceSize bufferSize = sizeof(uint32_t) * m_Indices.size();
            VulkanBuffer stagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            stagingBuffer.MapMemory(bufferSize);
            stagingBuffer.WriteToBuffer(m_Indices.data(), bufferSize, 0);
            stagingBuffer.UnmapMemory();

            m_IndexBuffer = std::make_unique<VulkanBuffer>(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VulkanRenderCommand::CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
        }
    }

    StaticMesh::~StaticMesh()
    {
    }
}
