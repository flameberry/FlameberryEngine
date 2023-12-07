#pragma once

#include <string>
#include <list>
#include <unordered_map>

#include "Core/UUID.h"
#include "Core/Core.h"

#include "Renderer/StaticMesh.h"
#include "Renderer/Material.h"

#include "AssetLoader.h"

namespace Flameberry {

    class AssetManager
    {
    public:
        template <typename Type>
        static std::shared_ptr<Type> TryGetOrLoadAsset(const std::filesystem::path& path)
        {
            FBY_ASSERT(!path.empty(), "Given file path is empty!");
            if (s_AssetFilePathToUUIDTable.find(path) != s_AssetFilePathToUUIDTable.end())
            {
                auto asset = s_AssetTable[s_AssetFilePathToUUIDTable[path]];
                FBY_ASSERT(Type::GetStaticAssetType() == asset->GetAssetType(), "Requested Asset Type doesn't match the asset type loaded in memory!");

                // Moving the asset handle to the "front" of the list to indicate that this asset is recently accessed
                // s_AssetCache.splice(s_AssetCache.begin(), s_AssetCache, asset->CacheIterator);

                FBY_INFO("Found asset having filepath: {} in asset table!", path);
                return std::static_pointer_cast<Type>(asset);
            }

            auto asset = AssetLoader::LoadAsset(path, Type::GetStaticAssetType());
            FBY_ASSERT(Type::GetStaticAssetType() == asset->GetAssetType(), "Requested Asset Type doesn't match the asset type loaded in memory!");

            s_AssetTable[asset->Handle] = asset;
            s_AssetFilePathToUUIDTable[path] = asset->Handle;

            // Moving the asset handle to the "front" of the list to indicate that this asset is recently accessed
            // s_AssetCache.splice(s_AssetCache.begin(), s_AssetCache, asset->CacheIterator);
            // Remove least recently used asset if max cache size is exceeded here
            return std::static_pointer_cast<Type>(asset);
        }

        template <typename Type>
        static std::shared_ptr<Type> GetAsset(AssetHandle handle)
        {
            if (s_AssetTable.find(handle) != s_AssetTable.end())
            {
                auto asset = s_AssetTable[handle];
                FBY_ASSERT(Type::GetStaticAssetType() == asset->GetAssetType(), "Requested Asset Type doesn't match the asset type loaded in memory!");

                // Moving the asset handle to the "front" of the list to indicate that this asset is recently accessed
                // s_AssetCache.splice(s_AssetCache.begin(), s_AssetCache, asset->CacheIterator);

                return std::static_pointer_cast<Type>(asset);
            }
//            FBY_WARN("Failed to find the asset with handle: {}", handle);
            return nullptr;
        }

        static void RegisterAsset(const std::shared_ptr<Asset>& asset)
        {
            if (s_AssetTable.find(asset->Handle) == s_AssetTable.end())
            {
                s_AssetTable[asset->Handle] = asset;
                if (!asset->FilePath.empty())
                    s_AssetFilePathToUUIDTable[asset->FilePath] = asset->Handle;
                return;
            }
            FBY_WARN("Failed to register asset with UUID: {}, Asset already registered!", (uint64_t)asset->Handle);
        }

        static bool IsAssetHandleValid(AssetHandle handle)
        {
            return handle != 0 && s_AssetTable.find(handle) != s_AssetTable.end();
        }

        static bool IsAssetLoaded(const std::string& path)
        {
            return s_AssetFilePathToUUIDTable.find(path) != s_AssetFilePathToUUIDTable.end();
        }

        static inline void Clear()
        {
            s_AssetTable.clear();
            s_AssetFilePathToUUIDTable.clear();
            s_AssetCache.clear();
        }

        static const std::unordered_map<AssetHandle, std::shared_ptr<Asset>>& GetAssetTable() { return s_AssetTable; }
    private:
        inline static std::unordered_map<AssetHandle, std::shared_ptr<Asset>> s_AssetTable;
        inline static std::unordered_map<std::string, AssetHandle> s_AssetFilePathToUUIDTable;

        inline static std::list<AssetHandle> s_AssetCache; // TODO: Implement this
        inline static const std::size_t s_MaxCPUCacheSizeInBytes = 4096, s_MaxGPUCacheSizeInBytes = 4096;
    };

}
