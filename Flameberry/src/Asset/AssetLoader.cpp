#include "AssetLoader.h"

#include "Core/Timer.h"
#include "AssetManager.h"
#include "Renderer/RenderCommand.h"

#include "TextureLoader.h"
#include "MeshLoader.h"
#include "MaterialLoader.h"

namespace Flameberry {
    struct AssetLoaderFunctionMapEntry {
        AssetType Type;
        std::shared_ptr<Asset>(*LoaderFunction)(const std::filesystem::path&);
    };

    constexpr AssetLoaderFunctionMapEntry g_AssetLoaderFunctionMap[4] = {
        { AssetType::None,       nullptr },
        { AssetType::Texture2D,  TextureLoader::LoadTexture2D },
        { AssetType::StaticMesh, MeshLoader::LoadMesh },
        { AssetType::Material,   MaterialLoader::LoadMaterial }
    };

    std::shared_ptr<Asset> AssetLoader::LoadAsset(const std::filesystem::path& path, AssetType type)
    {
        if ((uint16_t)type >= 4)
        {
            FBY_ERROR("Loader for the requested asset not present!");
            return nullptr;
        }
        return g_AssetLoaderFunctionMap[(uint16_t)type].LoaderFunction(path);
    }
}
