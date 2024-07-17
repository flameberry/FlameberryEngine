#pragma once

#include <filesystem>

#include "Asset/IAssetManager.h"
#include "Asset/AssetMetadata.h"

namespace Flameberry {

	using AssetRegistry = std::map<AssetHandle, AssetMetadata>;

	class EditorAssetManager : public IAssetManager
	{
	public:
		Ref<Asset> GetAsset(AssetHandle handle) override;
		bool IsAssetHandleValid(AssetHandle handle) const override;
		bool IsAssetLoaded(AssetHandle handle) const override;

		bool IsMemoryAsset(AssetHandle handle) const { return m_MemoryOnlyAssets.find(handle) != m_MemoryOnlyAssets.end(); }

		AssetHandle ImportAsset(const std::filesystem::path& filePath);
		void AddMemoryOnlyAsset(const Ref<Asset>& asset);

		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }
		const AssetMap& GetLoadedAssets() const { return m_LoadedAssets; }
		const AssetMetadata& GetAssetMetadata(AssetHandle handle) const { return m_AssetRegistry.at(handle); }

		void SerializeAssetRegistry();
		bool DeserializeAssetRegistry(const std::filesystem::path& path);

	private:
		AssetMap m_LoadedAssets;
		AssetMap m_MemoryOnlyAssets;
		AssetRegistry m_AssetRegistry;

		// For Caching purposes only
		// This is like a reverse AssetRegistry which stores AssetFilePath -> AssetHandle
		// Used for avoiding loading assets like Scene multiple times as separate assets
		// Because Scenes in Editor are loaded using filepaths...
		// and there should be a way to trace a filepath back to it's AssetHandle if it exists
		std::unordered_map<std::filesystem::path, AssetHandle> m_FilePathToAssetHandle;
	};

} // namespace Flameberry
