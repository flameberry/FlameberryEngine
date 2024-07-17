#pragma once

#include "Core/UUID.h"

#define FBY_DECLARE_ASSET_TYPE(Type)                              \
	::Flameberry::AssetType GetAssetType() const override         \
	{                                                             \
		return Type;                                              \
	}                                                             \
	static constexpr ::Flameberry::AssetType GetStaticAssetType() \
	{                                                             \
		return Type;                                              \
	}

namespace Flameberry {

	using AssetHandle = UUID;

	enum class AssetType : uint16_t
	{
		None = 0,
		Texture2D,
		StaticMesh,
		Material,
		Skymap,
		Font,
	};

	enum class AssetFlag : uint32_t
	{
		None = 0,
	};

	typedef uint32_t AssetFlags;

	class Asset
	{
	public:
		AssetHandle Handle;

		AssetFlags Flags = 0;
		virtual AssetType GetAssetType() const = 0;
	};

	namespace Utils {

		// Helper functions
		AssetType GetAssetTypeFromFileExtension(const std::string& extension);
		AssetType AssetTypeStringToEnum(const std::string& typeStr);
		std::string AssetTypeEnumToString(AssetType assetType);

	} // namespace Utils

} // namespace Flameberry
