#pragma once

#include "Renderer/StaticMesh.h"
#include "Renderer/Material.h"

namespace Flameberry {
	class AssetLoader
	{
	public:
		static Ref<Asset> LoadAsset(const std::filesystem::path& path, AssetType type);
	};
} // namespace Flameberry