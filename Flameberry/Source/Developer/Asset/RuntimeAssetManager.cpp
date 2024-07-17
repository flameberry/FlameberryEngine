#include "RuntimeAssetManager.h"

namespace Flameberry {

	Ref<Asset> RuntimeAssetManager::GetAsset(AssetHandle handle)
	{
		FBY_ASSERT(false, "RuntimeAssetManager::GetAsset(): Not implemented yet!");
		return nullptr;
	}

	bool RuntimeAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		FBY_ASSERT(false, "RuntimeAssetManager::IsAssetHandleValid(): Not implemented yet!");
		return false;
	}

	bool RuntimeAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		FBY_ASSERT(false, "RuntimeAssetManager::IsAssetLoaded(): Not implemented yet!");
		return false;
	}

} // namespace Flameberry
