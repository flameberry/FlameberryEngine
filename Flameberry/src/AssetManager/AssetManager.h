#pragma once

#include <unordered_map>
#include <string>

#include "Core/UUID.h"
#include "Core/Core.h"

#include "Renderer/Vulkan/StaticMesh.h"
#include "Renderer/Material.h"

namespace Flameberry {
    enum class AssetType { NONE = 0, TEXTURE, STATIC_MESH, MATERIAL };

    template <typename Type> constexpr AssetType GetAssetTypeEnumFromType() {
        if (std::is_same_v<Type, Texture2D>) return AssetType::TEXTURE;
        if (std::is_same_v<Type, StaticMesh>) return AssetType::STATIC_MESH;
        if (std::is_same_v<Type, Material>) return AssetType::MATERIAL;
        return AssetType::NONE;
    }

    struct Asset
    {
        AssetType Type;
        std::shared_ptr<void> AssetRef;
    };

    class AssetManager
    {
    public:
        template <typename Tp, typename... Args>
        static std::shared_ptr<Tp> TryGetOrLoadAssetFromFile(const std::string& path, Args... args)
        {
            constexpr AssetType assetType = GetAssetTypeEnumFromType<Tp>();
            static_assert(assetType != AssetType::NONE, "Cannot load asset of invalid Asset Type from file!");

            FL_ASSERT(!path.empty(), "Given file path is empty!");
            if (s_AssetFilePathToUUIDTable.find(path) != s_AssetFilePathToUUIDTable.end())
            {
                auto& asset = s_AssetTable[s_AssetFilePathToUUIDTable[path]];
                FL_ASSERT(assetType == asset.Type, "Asset Type requested doesn't match the asset type loaded in memory!");
                FL_INFO("Found asset having filepath: {0} in asset table!", path);
                return std::static_pointer_cast<Tp>(asset.AssetRef);
            }

            auto asset = Tp::LoadFromFile(path.c_str());
            s_AssetTable[asset->GetUUID()] = { assetType, asset };
            s_AssetFilePathToUUIDTable[path] = asset->GetUUID();
            return asset;
        }

        // Warning: Should not be used for creating assets that need to be loaded from disk
        // Note: Use TryGetOrLoadAssetFromFile() to load any asset from disk
        template <typename Type, typename... Args>
        static std::shared_ptr<Type> CreateAsset(Args... args)
        {
            constexpr AssetType assetType = GetAssetTypeEnumFromType<Type>();
            static_assert(assetType != AssetType::NONE, "Cannot create asset of invalid Asset Type!");

            auto asset = std::make_shared<Type>(std::forward<Args>(args)...);
            s_AssetTable[asset->GetUUID()] = { assetType, asset };
            return asset;
        }

        template <typename Type>
        static std::shared_ptr<Type> GetAsset(UUID handle)
        {
            constexpr AssetType assetType = GetAssetTypeEnumFromType<Type>();
            static_assert(assetType != AssetType::NONE, "Cannot get asset of invalid Asset Type!");

            if (s_AssetTable.find(handle) != s_AssetTable.end())
            {
                auto& asset = s_AssetTable[handle];
                FL_ASSERT(assetType == asset.Type, "Asset Type requested doesn't match the asset type loaded in memory!");
                return std::static_pointer_cast<Type>(asset.AssetRef);
            }
            FL_ERROR("Failed to find the asset with handle: {0}", handle);
            return nullptr;
        }

        template <typename Type>
        static void RegisterAsset(const std::shared_ptr<Type>& asset, const std::string& path = "")
        {
            constexpr AssetType assetType = GetAssetTypeEnumFromType<Type>();
            static_assert(assetType != AssetType::NONE, "Cannot get asset of invalid Asset Type!");

            if (s_AssetTable.find(asset->GetUUID()) == s_AssetTable.end())
            {
                s_AssetTable[asset->GetUUID()] = { assetType, asset };
                if (!path.empty())
                    s_AssetFilePathToUUIDTable[path] = asset->GetUUID();
                return;
            }
            FL_WARN("Failed to register asset with UUID: {0}, Asset already registered!", asset->GetUUID());

        }

        static bool IsAssetHandleValid(UUID handle)
        {
            return s_AssetTable.find(handle) != s_AssetTable.end();
        }

        static bool DoesAssetWithFilePathExist(const std::string& path)
        {
            return s_AssetFilePathToUUIDTable.find(path) != s_AssetFilePathToUUIDTable.end();
        }

        static inline void DestroyAssets()
        {
            s_AssetTable.clear();
            s_AssetFilePathToUUIDTable.clear();
        }

        static const std::unordered_map<UUID, Asset>& GetAssetTable() { return s_AssetTable; }
    private:
        inline static std::unordered_map<UUID, Asset> s_AssetTable;
        inline static std::unordered_map<std::string, UUID> s_AssetFilePathToUUIDTable; // TODO: Ensure to hash relative file paths
    };
}
