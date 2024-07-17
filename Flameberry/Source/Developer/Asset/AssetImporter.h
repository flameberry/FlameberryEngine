#pragma once

#include <filesystem>

#include "Core/Core.h"

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"

namespace Flameberry {

	class AssetImporter
	{
	public:
		static Ref<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
	};

} // namespace Flameberry
