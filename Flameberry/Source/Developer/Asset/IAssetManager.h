#pragma once

#include <map>

#include "Core/Core.h"
#include "Asset.h"
#include "AssetThread.h"

namespace Flameberry {

	using AssetMap = std::map<AssetHandle, Ref<Asset>>;

	class IAssetManager
	{
	public:
		IAssetManager()
			: m_AssetThread(CreateUnique<AssetThread>()) {}

		virtual ~IAssetManager() = default;
		virtual Ref<Asset> GetAsset(AssetHandle handle) = 0;
		virtual Ref<Asset> GetAssetAsync(AssetHandle handle) = 0;
		virtual bool IsAssetHandleValid(AssetHandle handle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) const = 0;

	protected:
		Unique<AssetThread> m_AssetThread;
	};

} // namespace Flameberry
