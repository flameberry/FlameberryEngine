#pragma once

#include <memory>
#include <string>
#include <list>
#include <filesystem>

#include "Core/UUID.h"

namespace Flameberry {
    using AssetHandle = UUID;

    enum class AssetType : uint16_t
    {
        None = 0,
        Texture2D, StaticMesh, Material
    };

    enum AssetFlag : uint16_t
    {
        None = 0,
    };

    typedef uint32_t AssetFlags;

    class Asset
    {
    public:
        AssetHandle Handle;

        AssetFlags Flags = 0;
        std::filesystem::path FilePath;
        std::list<AssetHandle>::iterator CacheIterator;
        std::size_t SizeInBytesOnCPU, SizeInBytesOnGPU;

        virtual AssetType GetAssetType() const = 0;
    };
}
