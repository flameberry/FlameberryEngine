#pragma once

#include <filesystem>

#include "Texture2D.h"

namespace Flameberry {

	struct MSDFFontData;

	class Font
	{
	public:
		Font(const std::filesystem::path& path);
		~Font();

		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
		const MSDFFontData* GetMSDFFontData() const { return m_Data; }

	private:
		MSDFFontData* m_Data;
		Ref<Texture2D> m_AtlasTexture;
	};

} // namespace Flameberry
