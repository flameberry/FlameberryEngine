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
		AssetThread();
		~AssetThread();

		Ref<Asset> QueueLoad(AssetHandle handle, const AssetMetadata& metadata);

	private:
		void Main();

	private:
		struct AssetLoadParameters
		{
			AssetHandle Handle;
			AssetMetadata Metadata;
		};

	private:
		std::atomic<bool> m_Running;

		// The assets whose load is pending are kept here
		std::queue<AssetLoadParameters> m_AssetLoadParametersQueue;
		std::mutex m_AssetLoadQueueMutex;

		// The assets which are loaded and ready are stored here until they are returned to the requester
		std::unordered_map<AssetHandle, Ref<Asset>> m_ReadyAssetMap;
		std::mutex m_ReadyAssetMapMutex;

		std::thread m_AssetThread;
	};

} // namespace Flameberry
