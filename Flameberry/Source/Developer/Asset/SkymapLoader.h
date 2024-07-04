#pragma once

#include <filesystem>

#include "Asset.h"
#include "Core/Core.h"

namespace Flameberry {

	class SkymapLoader
	{
	public:
		static Ref<Asset> LoadSkymap(const std::filesystem::path& path);
	};

} // namespace Flameberry
