#pragma once

#include <map>

#include "Core/Core.h"
#include "Asset.h"

namespace Flameberry {

	using AssetMap = std::map<AssetHandle, Ref<Asset>>;

	class IAssetManager
	{
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) = 0;
		virtual bool IsAssetHandleValid(AssetHandle handle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) const = 0;
	};

} // namespace Flameberry
