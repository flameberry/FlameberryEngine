#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

#include "VulkanVertex.h"
#include "VulkanBuffer.h"

namespace Flameberry {
    /// @brief This class deals with Static Meshes, i.e., 
    /// the vertex data can't be modified, but allows for efficient caching of Meshes
    class VulkanMesh
    {
    public:
        VulkanMesh(const std::string& path);
        VulkanMesh(const std::vector<VulkanVertex>& vertices, const std::vector<uint32_t>& indices);
        ~VulkanMesh();

        void Bind(VkCommandBuffer commandBuffer) const;
        void OnDraw(VkCommandBuffer commandBuffer) const;

        std::string GetName() const { return m_Name; }
        std::string GetFilePath() const { return m_FilePath; }

        static std::shared_ptr<VulkanMesh> TryGetOrLoadMesh(const std::string& path);
        static void ClearCache() { s_MeshCacheDirectory.clear(); }
    private:
        void CreateBuffers();
    private:
        std::vector<VulkanVertex> m_Vertices;
        std::vector<uint32_t> m_Indices;

        std::unique_ptr<VulkanBuffer> m_VertexBuffer, m_IndexBuffer;
        std::string m_Name = "default_mesh";
        std::string m_FilePath;

        static std::unordered_map<std::string, std::shared_ptr<VulkanMesh>> s_MeshCacheDirectory;
    };
}