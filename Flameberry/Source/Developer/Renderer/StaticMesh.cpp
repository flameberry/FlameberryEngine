#include "StaticMesh.h"

#include "Core/Core.h"
#include "Core/Timer.h"

#include "Asset/AssetManager.h"
#include "RenderCommand.h"
#include "Renderer/Material.h"
#include "Renderer/Renderer.h"

namespace Flameberry {
	StaticMesh::StaticMesh(const Ref<Buffer>& vertexBuffer,
		const Ref<Buffer>& indexBuffer,
		const std::vector<SubMesh>& submeshes)
		: m_VertexBuffer(vertexBuffer), m_IndexBuffer(indexBuffer), m_SubMeshes(std::move(submeshes)) {}

	StaticMesh::~StaticMesh() {}
} // namespace Flameberry
