#pragma once

#include <filesystem>

#include "Core/Core.h"
#include "Asset.h"

namespace Flameberry {

    class SkymapLoader
    {
    public:
        static Ref<Asset> LoadSkymap(const std::filesystem::path& path);
    };

}
