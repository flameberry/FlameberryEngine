#include "SkymapImporter.h"

#include "Renderer/Skymap.h"

namespace Flameberry {

	Ref<Skymap> SkymapImporter::ImportSkymap(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadSkymap(metadata.FilePath);
	}

	Ref<Skymap> SkymapImporter::LoadSkymap(const std::filesystem::path& path)
	{
		return CreateRef<Skymap>(path);
	}

} // namespace Flameberry