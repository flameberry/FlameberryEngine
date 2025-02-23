#include "AssetThread.h"

#include "AssetImporter.h"

namespace Flameberry {

	AssetThread::AssetThread()
		: m_Running(true), m_AssetThread(&AssetThread::Main, this)
	{
	}

	AssetThread::~AssetThread()
	{
		m_Running = false;
		m_AssetThread.join();
	}

	Ref<Asset> AssetThread::QueueLoad(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::scoped_lock readyLock(m_ReadyAssetMapMutex);
		{
			// Check if the asset is present in the ready queue
			if (auto it = m_ReadyAssetMap.find(handle); it != m_ReadyAssetMap.end())
			{
				// Check if asset present in the ready queue is loaded yet, i.e., if it's not nullptr
				if (Ref<Asset> loadedAsset = it->second)
				{
					m_ReadyAssetMap.erase(handle);
					return loadedAsset;
				}
				// The asset is present in the ready queue but is nullptr, i.e., not loaded yet
				// Here we return nullptr instead of adding it to the queue twice.
				return nullptr;
			}

			{
				// Acquire mutex
				std::scoped_lock loadQueueLock(m_AssetLoadQueueMutex);

				// Append asset load to queue
				m_AssetLoadParametersQueue.emplace(AssetLoadParameters{ handle, metadata });

				// This is to indicate that the asset has been added to the load queue but has not been loaded yet.
				m_ReadyAssetMap[handle] = nullptr;
			}
		}

		// The asset is not loaded yet.
		return nullptr;
	}

	void AssetThread::Main()
	{
		while (m_Running)
		{
			bool isLoadPending = false;
			AssetLoadParameters loadParameters;

			{
				// Acquire queue mutex
				std::scoped_lock loadQueueLock(m_AssetLoadQueueMutex);

				if (!m_AssetLoadParametersQueue.empty())
				{
					loadParameters = m_AssetLoadParametersQueue.front();
					m_AssetLoadParametersQueue.pop();
					isLoadPending = true;
				}
			}

			if (isLoadPending)
			{
				// Load the asset here
				Ref<Asset> loadedAsset = AssetImporter::ImportAsset(loadParameters.Handle, loadParameters.Metadata);

				std::scoped_lock readyLock(m_ReadyAssetMapMutex);
				m_ReadyAssetMap[loadParameters.Handle] = loadedAsset;
			}
		}
	}

} // namespace Flameberry