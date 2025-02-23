#pragma once

#include <thread>
#include <mutex>

#include "Core/Core.h"

#include "Asset.h"
#include "AssetMetadata.h"

namespace Flameberry {

	class AssetThread
	{
	public:
		static Ref<Asset> QueueLoad(AssetHandle handle, const AssetMetadata& metadata);

	private:
		static void Main();

	private:
		struct AssetLoadParameters
		{
			AssetHandle Handle;
			AssetMetadata Metadata;
		};

	private:
		// The assets whose load is pending are kept here
		static std::queue<AssetLoadParameters> s_AssetLoadParametersQueue;
		static std::mutex s_AssetLoadQueueMutex;

		// The assets which are loaded and ready are stored here until they are returned to the requester
		static std::unordered_map<AssetHandle, Ref<Asset>> s_ReadyAssetMap;
		static std::mutex s_ReadyAssetMapMutex;

		static std::thread s_AssetThread;
	};

} // namespace Flameberry
