#pragma once

#include <filesystem>

#include "Asset/Asset.h"
#include "Texture2D.h"

namespace Flameberry {

	struct MSDFFontData;

	class Font : public Asset
	{
	public:
		Font(const std::filesystem::path& path);
		~Font();

		const std::string& GetName() const { return m_Name; }
		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
		const MSDFFontData* GetMSDFFontData() const { return m_Data; }

		static Ref<Font> GetDefault();
		// Temporary solution to not having an Application Destroy Queue
		static void DestroyDefault();

		FBY_DECLARE_ASSET_TYPE(AssetType::Font);

	private:
		std::string m_Name;

		MSDFFontData* m_Data;
		Ref<Texture2D> m_AtlasTexture;
	};

} // namespace Flameberry
