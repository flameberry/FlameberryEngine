#pragma once

#include <filesystem>

#include "Core/Core.h"
#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"
#include "Renderer/Font.h"

namespace Flameberry {

	class FontImporter
	{
	public:
		static Ref<Font> ImportFont(AssetHandle handle, const AssetMetadata& metadata);
		static Ref<Font> LoadFont(const std::filesystem::path& path);
	};

} // namespace Flameberry
