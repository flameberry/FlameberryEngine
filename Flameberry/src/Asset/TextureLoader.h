#pragma once

#include "Renderer/Texture2D.h"

namespace Flameberry {
    class TextureLoader
    {
    public:
        static std::shared_ptr<Asset> LoadTexture2D(const std::filesystem::path& path);
    };
}
