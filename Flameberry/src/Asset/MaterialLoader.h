#pragma once

#include "Renderer/Material.h"

namespace Flameberry {
    class MaterialLoader
    {
    public:
        static std::shared_ptr<Asset> LoadMaterial(const std::filesystem::path& path);
    };
}
