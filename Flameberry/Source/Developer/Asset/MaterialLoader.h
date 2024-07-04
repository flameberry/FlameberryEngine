#pragma once

#include "Renderer/MaterialAsset.h"

namespace Flameberry {
	class MaterialLoader
	{
	public:
		static Ref<Asset> LoadMaterial(const std::filesystem::path& path);
	};
} // namespace Flameberry
