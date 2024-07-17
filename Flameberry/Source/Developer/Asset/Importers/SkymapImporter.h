#pragma once

#include <filesystem>

#include "Core/Core.h"
#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"
#include "Renderer/Skymap.h"

namespace Flameberry {

	class SkymapImporter
	{
	public:
		static Ref<Skymap> ImportSkymap(AssetHandle handle, const AssetMetadata& metadata);
		static Ref<Skymap> LoadSkymap(const std::filesystem::path& path);
	};

} // namespace Flameberry
