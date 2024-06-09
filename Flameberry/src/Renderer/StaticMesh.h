#pragma once

#include <vector>
#include <string>

#include "Buffer.h"
#include "Asset/Asset.h"
#include "AABB.h"

namespace Flameberry {

    struct SubMesh
    {
        AssetHandle MaterialHandle = 0;
        uint32_t IndexOffset, IndexCount;
        AABB AABB;
    };

    /// @brief This class deals with Static Meshes, i.e., 
    /// the vertex data can't be modified, but allows for efficient caching of Meshes
    class StaticMesh : public Asset
    {
    public:
        StaticMesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer, const std::vector<SubMesh>& submeshes);
        ~StaticMesh();

        std::string GetName() const { return m_Name; }
        const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
        const Ref<Buffer>& GetVertexBuffer() const { return m_VertexBuffer; }
        const Ref<Buffer>& GetIndexBuffer() const { return m_IndexBuffer; }

        FBY_DECLARE_ASSET_TYPE(AssetType::StaticMesh);
    private:
        Ref<Buffer> m_VertexBuffer, m_IndexBuffer;
        std::vector<SubMesh> m_SubMeshes;

        std::string m_Name = "StaticMesh";
        friend class SceneSerializer;
    };

}
