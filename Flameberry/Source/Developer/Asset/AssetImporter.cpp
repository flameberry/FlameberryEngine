#include "AssetImporter.h"

#include "Importers/SceneImporter.h"
#include "Importers/TextureImporter.h"
#include "Importers/MeshImporter.h"
#include "Importers/MaterialImporter.h"
#include "Importers/SkymapImporter.h"
#include "Importers/FontImporter.h"

namespace Flameberry {

	struct AssetImporterFunctionMapEntry
	{
		AssetType Type;
		std::function<Ref<Asset>(AssetHandle, const AssetMetadata&)> ImporterFunction;
	};

	static const AssetImporterFunctionMapEntry g_AssetImporterFunctionMap[] = {
		{ AssetType::None, nullptr },
		{ AssetType::Scene, SceneImporter::ImportScene },
		{ AssetType::Texture2D, TextureImporter::ImportTexture2D },
		{ AssetType::StaticMesh, MeshImporter::ImportMesh },
		{ AssetType::Material, MaterialImporter::ImportMaterial },
		{ AssetType::Skymap, SkymapImporter::ImportSkymap },
		{ AssetType::Font, FontImporter::ImportFont },
	};

	Ref<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		if (metadata.Type == AssetType::None || (uint16_t)metadata.Type >= sizeof(g_AssetImporterFunctionMap) / sizeof(AssetImporterFunctionMapEntry))
		{
			FBY_ERROR("Importer for the requested asset not present!");
			return nullptr;
		}
		return g_AssetImporterFunctionMap[(uint16_t)metadata.Type].ImporterFunction(handle, metadata);
	}

} // namespace Flameberry
