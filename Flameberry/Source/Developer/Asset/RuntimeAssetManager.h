#pragma once

#include "IAssetManager.h"

namespace Flameberry {

	class RuntimeAssetManager : public IAssetManager
	{
	public:
		Ref<Asset> GetAsset(AssetHandle handle) override;
		bool IsAssetHandleValid(AssetHandle handle) const override;
		bool IsAssetLoaded(AssetHandle handle) const override;
	};

} // namespace Flameberry
