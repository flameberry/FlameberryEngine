#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

#include "VulkanVertex.h"
#include "Buffer.h"
#include "Asset/Asset.h"

namespace Flameberry {
    struct SubMesh
    {
        AssetHandle MaterialHandle = 0;
        uint32_t IndexOffset, IndexCount;
    };

    /// @brief This class deals with Static Meshes, i.e., 
    /// the vertex data can't be modified, but allows for efficient caching of Meshes
    class StaticMesh : public Asset
    {
    public:
        StaticMesh(const std::shared_ptr<Buffer>& vertexBuffer, const std::shared_ptr<Buffer>& indexBuffer, const std::vector<SubMesh>& submeshes);
        ~StaticMesh();

        void Bind() const;
        void OnDraw() const;
        void OnDrawSubMesh(uint32_t subMeshIndex) const;

        std::string GetName() const { return m_Name; }
        const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }

        AssetType GetAssetType() const override { return AssetType::StaticMesh; }
        static constexpr AssetType GetStaticAssetType() { return AssetType::StaticMesh; }
    private:
        std::shared_ptr<Buffer> m_VertexBuffer, m_IndexBuffer;
        std::vector<SubMesh> m_SubMeshes;

        std::string m_Name = "StaticMesh";
        friend class SceneSerializer;
    };
}
