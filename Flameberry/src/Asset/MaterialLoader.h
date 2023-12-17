#pragma once

#include "Renderer/Material.h"

namespace Flameberry {
    class MaterialLoader
    {
    public:
        static Ref<Asset> LoadMaterial(const std::filesystem::path& path);
    };
}
