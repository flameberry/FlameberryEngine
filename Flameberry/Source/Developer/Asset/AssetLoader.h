#pragma once

#include "Renderer/Material.h"
#include "Renderer/StaticMesh.h"

namespace Flameberry {
	class AssetLoader
	{
	public:
		static Ref<Asset> LoadAsset(const std::filesystem::path& path,
			AssetType type);
	};
} // namespace Flameberry