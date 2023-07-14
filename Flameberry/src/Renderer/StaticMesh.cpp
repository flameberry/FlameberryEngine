#include "StaticMesh.h"

#include "Core/Core.h"
#include "Core/Timer.h"

#include "RenderCommand.h"
#include "Renderer/Material.h"
#include "Renderer/Renderer.h"
#include "Asset/AssetManager.h"

namespace Flameberry {
    StaticMesh::StaticMesh(const std::shared_ptr<Buffer>& vertexBuffer, const std::shared_ptr<Buffer>& indexBuffer, const std::vector<SubMesh>& submeshes)
        : m_VertexBuffer(vertexBuffer), m_IndexBuffer(indexBuffer), m_SubMeshes(std::move(submeshes))
    {
    }

    void StaticMesh::Bind() const
    {
        Renderer::Submit([vertexBuffer = m_VertexBuffer->GetBuffer(), indexBuffer = m_IndexBuffer->GetBuffer()](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
                vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }
        );
    }

    void StaticMesh::OnDraw() const
    {
        const uint32_t size = m_SubMeshes.back().IndexOffset + m_SubMeshes.back().IndexCount;
        Renderer::Submit([size](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdDrawIndexed(cmdBuffer, size, 1, 0, 0, 0);
            }
        );
    }

    void StaticMesh::OnDrawSubMesh(uint32_t subMeshIndex) const
    {
        Renderer::Submit([submesh = m_SubMeshes[subMeshIndex]](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdDrawIndexed(cmdBuffer, submesh.IndexCount, 1, submesh.IndexOffset, 0, 0);
            }
        );
    }

    StaticMesh::~StaticMesh()
    {
    }
}
