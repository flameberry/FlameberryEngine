#include "Asset.h"

namespace Flameberry {

	namespace Utils {

		static std::unordered_map<std::string, AssetType> g_FileExtensionToAssetTypeMap = {
			// Scene
			{ ".berry", AssetType::Scene },

			// Texture2D
			{ ".png", AssetType::Texture2D },
			{ ".jpg", AssetType::Texture2D },
			{ ".bmp", AssetType::Texture2D },
			{ ".tga", AssetType::Texture2D },
			{ ".jpeg", AssetType::Texture2D },

			// Static Mesh
			{ ".gltf", AssetType::StaticMesh },
			{ ".obj", AssetType::StaticMesh },
			{ ".fbx", AssetType::StaticMesh },
			{ ".glb", AssetType::StaticMesh },
			{ ".stl", AssetType::StaticMesh },

			// Material
			{ ".fbmat", AssetType::Material },

			// Skymap
			{ ".hdr", AssetType::Skymap },

			// Font
			{ ".ttf", AssetType::Font },
		};

		AssetType GetAssetTypeFromFileExtension(const std::string& extension)
		{
			auto it = g_FileExtensionToAssetTypeMap.find(extension);
			AssetType assetType = it != g_FileExtensionToAssetTypeMap.end() ? it->second : AssetType::None;

			return assetType;
		}

		AssetType AssetTypeStringToEnum(const std::string& typeStr)
		{
			if (typeStr == "Scene")
				return AssetType::Scene;
			if (typeStr == "Texture2D")
				return AssetType::Texture2D;
			if (typeStr == "StaticMesh")
				return AssetType::StaticMesh;
			if (typeStr == "Material")
				return AssetType::Material;
			if (typeStr == "Skymap")
				return AssetType::Skymap;
			if (typeStr == "Font")
				return AssetType::Font;
		}

		std::string AssetTypeEnumToString(AssetType assetType)
		{
			switch (assetType)
			{
				case AssetType::Scene:
					return "Scene";
				case AssetType::Texture2D:
					return "Texture2D";
				case AssetType::StaticMesh:
					return "StaticMesh";
				case AssetType::Material:
					return "Material";
				case AssetType::Skymap:
					return "Skymap";
				case AssetType::Font:
					return "Font";
			}
		}

	} // namespace Utils

} // namespace Flameberry