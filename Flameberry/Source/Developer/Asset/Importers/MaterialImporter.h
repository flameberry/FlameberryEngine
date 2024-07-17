#pragma once

#include "Asset/AssetMetadata.h"
#include "Renderer/MaterialAsset.h"

namespace Flameberry {

	class MaterialImporter
	{
	public:
		static Ref<MaterialAsset> ImportMaterial(AssetHandle handle, const AssetMetadata& metadata);
		static Ref<MaterialAsset> LoadMaterial(const std::filesystem::path& path);
	};

} // namespace Flameberry
