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

    constexpr AssetLoaderFunctionMapEntry g_AssetLoaderFunctionMap[] = {
        { AssetType::None,       nullptr },
        { AssetType::Texture2D,  TextureLoader::LoadTexture2D },
        { AssetType::StaticMesh, MeshLoader::LoadMesh },
        { AssetType::Material,   MaterialLoader::LoadMaterial }
    };

    std::shared_ptr<Asset> AssetLoader::LoadAsset(const std::filesystem::path& path)
    {
        const auto& ext = path.extension();
        if (ext == ".obj")
            return g_AssetLoaderFunctionMap[(uint16_t)AssetType::StaticMesh].LoaderFunction(path);
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".hdr" || ext == ".tga" || ext == ".bmp")
            return g_AssetLoaderFunctionMap[(uint16_t)AssetType::Texture2D].LoaderFunction(path);
        if (ext == ".fbmat")
            return g_AssetLoaderFunctionMap[(uint16_t)AssetType::Material].LoaderFunction(path);

        FL_ASSERT(0, "Loader for the requested asset not present!");
    }
}
