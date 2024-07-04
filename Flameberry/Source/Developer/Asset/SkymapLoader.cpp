#include "SkymapLoader.h"

#include "Renderer/Skymap.h"

namespace Flameberry {

	Ref<Asset> SkymapLoader::LoadSkymap(const std::filesystem::path& path)
	{
		auto skymapAsset = CreateRef<Skymap>(path);

		// Set Asset Class Variables
		skymapAsset->FilePath = path;

		return skymapAsset;
	}

} // namespace Flameberry