#include "MaterialImporter.h"

namespace Flameberry {

	Ref<MaterialAsset> MaterialImporter::ImportMaterial(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadMaterial(metadata.FilePath);
	}

	Ref<MaterialAsset> MaterialImporter::LoadMaterial(const std::filesystem::path& path)
	{
		return MaterialAssetSerializer::Deserialize(path.c_str());
	}

} // namespace Flameberry
