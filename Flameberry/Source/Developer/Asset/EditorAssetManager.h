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

		const AssetMap& GetLoadedAssets() const { return m_LoadedAssets; }
		const AssetMetadata& GetAssetMetadata(AssetHandle handle) const { return m_AssetRegistry.at(handle); }

		void SerializeAssetRegistry();
		bool DeserializeAssetRegistry(const std::filesystem::path& path);

	private:
		AssetMap m_LoadedAssets;
		AssetMap m_MemoryOnlyAssets;
		AssetRegistry m_AssetRegistry;
	};

} // namespace Flameberry
