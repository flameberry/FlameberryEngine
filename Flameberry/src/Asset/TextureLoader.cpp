#include "TextureLoader.h"

namespace Flameberry {

    std::shared_ptr<Asset> Flameberry::TextureLoader::LoadTexture2D(const std::filesystem::path& path)
    {
        // TODO: Separate stbi_image loading functions from Constructor of Texture2D to this function
        auto asset = std::make_shared<Texture2D>(path);

        // Set Asset Class Variables
        asset->FilePath = path;
        asset->SizeInBytesOnCPU = sizeof(Texture2D);
        asset->SizeInBytesOnGPU = 0; // TODO: Calculate using channels * width * height * bytes_per_channel 
        return asset;
    }

}