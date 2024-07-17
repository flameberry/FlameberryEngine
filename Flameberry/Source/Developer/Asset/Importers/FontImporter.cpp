#include "FontImporter.h"

#include "Renderer/Font.h"
#include "FontImporter.h"

namespace Flameberry {

	Ref<Font> FontImporter::ImportFont(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadFont(metadata.FilePath);
	}

	Ref<Font> FontImporter::LoadFont(const std::filesystem::path& path)
	{
		return CreateRef<Font>(path);
	}

} // namespace Flameberry
