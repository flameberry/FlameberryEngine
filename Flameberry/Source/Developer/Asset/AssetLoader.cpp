#include "AssetLoader.h"

#include "AssetManager.h"
#include "Core/Timer.h"
#include "Renderer/RenderCommand.h"

#include "MaterialLoader.h"
#include "MeshLoader.h"
#include "SkymapLoader.h"
#include "TextureLoader.h"

namespace Flameberry {
	struct AssetLoaderFunctionMapEntry
	{
		AssetType Type;
		Ref<Asset> (*LoaderFunction)(const std::filesystem::path&);
	};

	constexpr AssetLoaderFunctionMapEntry g_AssetLoaderFunctionMap[] = {
		{ AssetType::None, nullptr },
		{ AssetType::Texture2D, TextureLoader::LoadTexture2D },
		{ AssetType::StaticMesh, MeshLoader::LoadMesh },
		{ AssetType::Material, MaterialLoader::LoadMaterial },
		{ AssetType::Skymap, SkymapLoader::LoadSkymap }
	};

	Ref<Asset> AssetLoader::LoadAsset(const std::filesystem::path& path,
		AssetType type)
	{
		if ((uint16_t)type >= sizeof(g_AssetLoaderFunctionMap) / sizeof(AssetLoaderFunctionMapEntry))
		{
			FBY_ERROR("Loader for the requested asset not present!");
			return nullptr;
		}
		return g_AssetLoaderFunctionMap[(uint16_t)type].LoaderFunction(path);
	}
} // namespace Flameberry
