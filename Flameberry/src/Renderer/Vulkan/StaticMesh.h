#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

#include "VulkanVertex.h"
#include "Buffer.h"

#include "Core/UUID.h"

namespace Flameberry {
    struct SubMesh
    {
        UUID MaterialUUID = 0;
        uint32_t IndexOffset, IndexCount;
    };

    /// @brief This class deals with Static Meshes, i.e., 
    /// the vertex data can't be modified, but allows for efficient caching of Meshes
    class StaticMesh
    {
    public:
        StaticMesh(const std::string& path);
        StaticMesh(const std::vector<VulkanVertex>& vertices, const std::vector<uint32_t>& indices);
        ~StaticMesh();

        void Bind(VkCommandBuffer commandBuffer) const;
        void OnDraw(VkCommandBuffer commandBuffer) const;
        void OnDrawSubMesh(VkCommandBuffer commandBuffer, uint32_t subMeshIndex) const;

        UUID GetUUID() const { return m_UUID; }
        std::string GetName() const { return m_Name; }
        std::string GetFilePath() const { return m_FilePath; }
        const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
        void SetMaterialToSubMesh(uint32_t submeshIndex, UUID materialID) { m_SubMeshes[submeshIndex].MaterialUUID = materialID; }

        static std::shared_ptr<StaticMesh> LoadFromFile(const char* path) { return std::make_shared<StaticMesh>(path); }
    private:
        void Load(const std::string& path);
        void LoadOBJ(const std::string& path);
        void LoadGLTF(const std::string& path, bool isBinary);
        void CreateBuffers();
    private:
        std::vector<VulkanVertex> m_Vertices; // TODO: Does it need to be stored?
        std::vector<uint32_t> m_Indices;

        std::unique_ptr<Buffer> m_VertexBuffer, m_IndexBuffer;
        std::string m_Name = "StaticMesh";
        std::string m_FilePath;

        std::vector<SubMesh> m_SubMeshes;
        UUID m_UUID;

        friend class SceneSerializer;
    };
}
