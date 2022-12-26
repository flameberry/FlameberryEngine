#include "VulkanMesh.h"

#include "VulkanRenderCommand.h"

namespace Flameberry {
    VulkanMesh::VulkanMesh(const std::vector<VulkanVertex>& vertices, const std::vector<uint32_t>& indices)
        : m_Vertices(vertices), m_Indices(indices)
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
            stagingBuffer.WriteToBuffer(indices.data(), bufferSize, 0);
            stagingBuffer.UnmapMemory();

            m_IndexBuffer = std::make_unique<VulkanBuffer>(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VulkanRenderCommand::CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
        }
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
        vkCmdDrawIndexed(commandBuffer, m_Indices.size(), 1, 0, 0, 0);
    }

    VulkanMesh::~VulkanMesh()
    {
    }
}