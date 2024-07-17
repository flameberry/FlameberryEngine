#pragma once

#include <filesystem>

#include "Asset.h"

namespace Flameberry {

	struct AssetMetadata
	{
		AssetType Type;
		std::string FilePath; // Used as Name for Memory Only assets

		bool IsMemoryAsset;
	};

} // namespace Flameberry
