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
		StaticMesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer, const std::vector<SubMesh>& submeshes, const AABB& aabb);
		~StaticMesh();

		inline void SetName(const std::string& name) { m_Name = name; }

		inline const AABB& GetAABB() const { return m_MeshAABB; }
		inline std::string GetName() const { return m_Name; }
		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		inline const Ref<Buffer>& GetVertexBuffer() const { return m_VertexBuffer; }
		inline const Ref<Buffer>& GetIndexBuffer() const { return m_IndexBuffer; }

		FBY_DECLARE_ASSET_TYPE(AssetType::StaticMesh);

	private:
		Ref<Buffer> m_VertexBuffer, m_IndexBuffer;
		std::vector<SubMesh> m_SubMeshes;

		// This must represent the
		AABB m_MeshAABB;

		std::string m_Name = "StaticMesh";
		friend class SceneSerializer;
	};

} // namespace Flameberry
