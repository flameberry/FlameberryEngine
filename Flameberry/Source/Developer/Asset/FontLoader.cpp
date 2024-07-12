#include "FontLoader.h"

#include "Renderer/Font.h"

namespace Flameberry {

	Ref<Asset> FontLoader::LoadFont(const std::filesystem::path& path)
	{
		Ref<Font> fontAsset = CreateRef<Font>(path);

		// Set Asset Class Variables
		fontAsset->FilePath = path;

		return fontAsset;
	}

} // namespace Flameberry
