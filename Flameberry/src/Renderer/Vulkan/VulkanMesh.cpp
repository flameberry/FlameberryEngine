#include "VulkanMesh.h"

#include "Core/Core.h"
#include "VulkanRenderCommand.h"

namespace Flameberry {
    std::unordered_map<std::string, std::shared_ptr<VulkanMesh>> VulkanMesh::s_MeshCacheDirectory;

    VulkanMesh::VulkanMesh(const std::string& path)
        : m_FilePath(path)
    {
        auto [vk_vertices, indices] = VulkanRenderCommand::LoadModel(path);
        m_Vertices = vk_vertices;
        m_Indices = indices;

        CreateBuffers();

        uint32_t lengthSlash = m_FilePath.find_last_of('/') + 1;
        uint32_t lengthDot = m_FilePath.find_last_of('.');
        m_Name = m_FilePath.substr(lengthSlash, lengthDot - lengthSlash);

        FL_TRACE("Allocated {0}, {1} bytes for {2}: Vertices, Indices", m_Vertices.size() * sizeof(VulkanVertex), m_Indices.size() * sizeof(uint32_t), m_Name);
    }

    VulkanMesh::VulkanMesh(const std::vector<VulkanVertex>& vertices, const std::vector<uint32_t>& indices)
        : m_Vertices(vertices), m_Indices(indices)
    {
        CreateBuffers();
    }

    void VulkanMesh::Bind(VkCommandBuffer commandBuffer) const
    {
        VkBuffer vk_vertex_buffers[] = { m_VertexBuffer->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vk_vertex_buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanMesh::OnDraw(VkCommandBuffer commandBuffer) const
    {
        vkCmdDrawIndexed(commandBuffer, (uint32_t)m_Indices.size(), 1, 0, 0, 0);
    }

    std::shared_ptr<VulkanMesh> VulkanMesh::TryGetOrLoadMesh(const std::string& path)
    {
        if (s_MeshCacheDirectory.find(path) != s_MeshCacheDirectory.end()) {
            FL_INFO("Successfully retrieved vulkan mesh from cache!");
            return s_MeshCacheDirectory[path];
        }
        else {
            std::shared_ptr<VulkanMesh> mesh = std::make_shared<VulkanMesh>(path.c_str());
            s_MeshCacheDirectory[path] = mesh;
            FL_INFO("Mesh not found in cache! Loading mesh from disk!");
            return mesh;
        }
    }

    void VulkanMesh::CreateBuffers()
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

    VulkanMesh::~VulkanMesh()
    {
    }
}
