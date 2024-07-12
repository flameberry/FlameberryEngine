#pragma once

#include <filesystem>

#include "Core/Core.h"
#include "Asset.h"

namespace Flameberry {

	class FontLoader
	{
	public:
		static Ref<Asset> LoadFont(const std::filesystem::path& path);

	private:
	};

} // namespace Flameberry
