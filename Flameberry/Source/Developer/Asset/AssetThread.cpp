#include "AssetThread.h"

#include "AssetImporter.h"

namespace Flameberry {

	std::thread AssetThread::s_AssetThread(&AssetThread::Main);

	std::queue<AssetThread::AssetLoadParameters> AssetThread::s_AssetLoadParametersQueue;
	std::mutex AssetThread::s_AssetLoadQueueMutex;

	std::unordered_map<AssetHandle, Ref<Asset>> AssetThread::s_ReadyAssetMap;
	std::mutex AssetThread::s_ReadyAssetMapMutex;

	Ref<Asset> AssetThread::QueueLoad(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::scoped_lock readyLock(s_ReadyAssetMapMutex);
		{
			// Check if the asset is present in the ready queue
			if (auto it = s_ReadyAssetMap.find(handle); it != s_ReadyAssetMap.end())
			{
				// Check if asset present in the ready queue is loaded yet, i.e., if it's not nullptr
				if (Ref<Asset> loadedAsset = it->second)
				{
					s_ReadyAssetMap.erase(handle);
					return loadedAsset;
				}
				// The asset is present in the ready queue but is nullptr, i.e., not loaded yet
				// Here we return nullptr instead of adding it to the queue twice.
				return nullptr;
			}

			{
				// Acquire mutex
				std::scoped_lock loadQueueLock(s_AssetLoadQueueMutex);

				// Append asset load to queue
				s_AssetLoadParametersQueue.emplace(AssetLoadParameters{ handle, metadata });

				// This is to indicate that the asset has been added to the load queue but has not been loaded yet.
				s_ReadyAssetMap[handle] = nullptr;
			}
		}

		// The asset is not loaded yet.
		return nullptr;
	}

	void AssetThread::Main()
	{
		while (true)
		{
			bool isLoadPending = false;
			AssetLoadParameters loadParameters;

			{
				// Acquire queue mutex
				std::scoped_lock loadQueueLock(s_AssetLoadQueueMutex);

				if (!s_AssetLoadParametersQueue.empty())
				{
					loadParameters = s_AssetLoadParametersQueue.front();
					s_AssetLoadParametersQueue.pop();
					isLoadPending = true;
				}
			}

			if (isLoadPending)
			{
				// Load the asset here
				Ref<Asset> loadedAsset = AssetImporter::ImportAsset(loadParameters.Handle, loadParameters.Metadata);

				std::scoped_lock readyLock(s_ReadyAssetMapMutex);
				s_ReadyAssetMap[loadParameters.Handle] = loadedAsset;
			}
		}
	}

} // namespace Flameberry