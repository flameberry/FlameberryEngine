#include "TextureLoader.h"

#include <stb_image/stb_image.h>

namespace Flameberry {

    std::shared_ptr<Asset> Flameberry::TextureLoader::LoadTexture2D(const std::filesystem::path& path)
    {
        // TODO: Separate stbi_image loading functions from Constructor of Texture2D to this function
        // auto asset = std::make_shared<Texture2D>(path);
        
        int width, height, channels, bytesPerChannel;
        
        void* pixels = nullptr;
        VkFormat format = VK_FORMAT_UNDEFINED;
        
        if (stbi_is_hdr(path.c_str()))
        {
            pixels = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            bytesPerChannel = 4;
        }
        else
        {
            pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            format = VK_FORMAT_R8G8B8A8_UNORM;
            bytesPerChannel = 1;
        }
        
        auto asset = std::make_shared<Texture2D>(pixels, width, height, bytesPerChannel, format);

        // Set Asset Class Variables
        asset->FilePath = path;
        asset->SizeInBytesOnCPU = sizeof(Texture2D);
        asset->SizeInBytesOnGPU = 0; // TODO: Calculate using channels * width * height * bytes_per_channel 
        return asset;
    }

}
