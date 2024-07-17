#pragma once

#include <filesystem>

#include "Asset/AssetMetadata.h"
#include "Renderer/StaticMesh.h"

namespace Flameberry {

	class MeshImporter
	{
	public:
		static Ref<StaticMesh> ImportMesh(AssetHandle handle, const AssetMetadata& metadata);
		static Ref<StaticMesh> LoadMesh(const std::filesystem::path& path);
	};

} // namespace Flameberry