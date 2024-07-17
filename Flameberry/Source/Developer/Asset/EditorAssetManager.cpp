#include "EditorAssetManager.h"

#include <fstream>
#include "Core/YamlUtils.h"

#include "Project/Project.h"
#include "Asset/AssetImporter.h"

namespace Flameberry {

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		return m_AssetRegistry.find(handle) != m_AssetRegistry.end();
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
	{
		// 1. Check if handle is valid
		if (!IsAssetHandleValid(handle))
			return nullptr;

		// Get the metadata
		const AssetMetadata& metadata = m_AssetRegistry.at(handle);

		Ref<Asset> asset;

		// 2. Check if the asset is MemoryOnly
		if (metadata.IsMemoryAsset && IsMemoryAsset(handle))
			return m_MemoryOnlyAssets.at(handle);

		// 3. Check if the asset is loaded
		if (IsAssetLoaded(handle))
		{
			asset = m_LoadedAssets.at(handle);
		}
		else
		{
			// 4. Load Asset if not already loaded
			asset = AssetImporter::ImportAsset(handle, metadata);

			// Set the already known AssetHandle for this asset
			asset->Handle = handle;

			m_LoadedAssets[asset->Handle] = asset;
		}

		// 5. Return the loaded asset
		return asset;
	}

	AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& filePath)
	{
		// Check if the asset with filePath is already present in the AssetRegistry
		if (const auto it = m_FilePathToAssetHandle.find(filePath); it != m_FilePathToAssetHandle.end())
			return it->second;

		// Import the asset
		const std::string& extension = filePath.extension().string();
		const AssetType type = Utils::GetAssetTypeFromFileExtension(extension);

		AssetMetadata metadata;
		metadata.Type = type;
		metadata.FilePath = filePath;
		metadata.IsMemoryAsset = false;

		// Currently it doesn't matter what the handle is as...
		// a new handle will be generated while importing asset
		AssetHandle handle = 0;

		if (Ref<Asset> asset = AssetImporter::ImportAsset(handle, metadata))
		{
			m_AssetRegistry[asset->Handle] = metadata;
			m_LoadedAssets[asset->Handle] = asset;

			// Cache the filepath to avoid loading duplicate assets
			// TODO: What if user intends to load the asset as a duplicate? Is that a valid concern?
			m_FilePathToAssetHandle[filePath] = asset->Handle;

			SerializeAssetRegistry();
			return asset->Handle;
		}
		return 0;
	}

	void EditorAssetManager::AddMemoryOnlyAsset(const Ref<Asset>& asset)
	{
		AssetMetadata metadata;
		metadata.Type = asset->GetAssetType();
		metadata.FilePath = "";
		metadata.IsMemoryAsset = true;

		m_AssetRegistry[asset->Handle] = metadata;
		m_MemoryOnlyAssets[asset->Handle] = asset;
	}

	void EditorAssetManager::SerializeAssetRegistry()
	{
		YAML::Emitter out;

		out << YAML::BeginMap; // Root
		out << YAML::Key << "AssetRegistry" << YAML::Value;

		out << YAML::BeginSeq;
		for (const auto& [handle, metadata] : m_AssetRegistry)
		{
			if (!metadata.IsMemoryAsset)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << handle;
				out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath;
				out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeEnumToString(metadata.Type);
				out << YAML::EndMap;
			}
		}
		out << YAML::EndSeq;
		out << YAML::EndMap; // Root

		std::filesystem::path path = Project::GetActiveProject()->GetAssetRegistryPath();

		std::ofstream fout(path);
		fout << out.c_str();
	}

	bool EditorAssetManager::DeserializeAssetRegistry(const std::filesystem::path& path)
	{
		std::ifstream in(path);
		std::stringstream ss;
		ss << in.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		YAML::Node root = data["AssetRegistry"];

		if (!root)
		{
			FBY_ERROR("Failed to deserialize Asset Registry: Wrong format");
			return false;
		}

		for (const auto& node : root)
		{
			AssetHandle handle = node["Handle"].as<AssetHandle>();
			auto& metadata = m_AssetRegistry[handle];
			metadata.FilePath = node["FilePath"].as<std::string>();
			metadata.Type = Utils::AssetTypeStringToEnum(node["Type"].as<std::string>());

			// Caching the filepath
			m_FilePathToAssetHandle[metadata.FilePath] = handle;
		}

		return true;
	}

} // namespace Flameberry
