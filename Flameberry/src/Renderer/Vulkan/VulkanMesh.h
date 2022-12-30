#pragma once

#include <vector>
#include <memory>

#include "VulkanVertex.h"
#include "VulkanBuffer.h"

namespace Flameberry {
    class VulkanMesh
    {
    public:
        VulkanMesh(const std::vector<VulkanVertex>& vertices, const std::vector<uint32_t>& indices);
        ~VulkanMesh();

        void Bind(VkCommandBuffer commandBuffer) const;
        void OnDraw(VkCommandBuffer commandBuffer) const;

        template<typename... Args>
        static std::shared_ptr<VulkanMesh> Create(Args... args) { return std::make_shared<VulkanMesh>(std::forward<Args>(args)...); }
    private:
        std::vector<VulkanVertex> m_Vertices;
        std::vector<uint32_t> m_Indices;

        std::unique_ptr<VulkanBuffer> m_VertexBuffer, m_IndexBuffer;
    };
}