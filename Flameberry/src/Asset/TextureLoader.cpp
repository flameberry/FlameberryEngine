#include "TextureLoader.h"

#include <stb_image/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image/stb_image_resize2.h>

namespace Flameberry {

    std::shared_ptr<Asset> TextureLoader::LoadTexture2D(const std::filesystem::path& path)
    {
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
        
        stbi_image_free(pixels);

        // Set Asset Class Variables
        asset->FilePath = path;
        asset->SizeInBytesOnCPU = sizeof(Texture2D);
        asset->SizeInBytesOnGPU = 0; // TODO: Calculate using channels * width * height * bytes_per_channel 
        return asset;
    }
    
    // TODO: Needs work quality wise
    std::shared_ptr<Texture2D> TextureLoader::LoadTexture2DResized(const std::filesystem::path &path, int newWidth, int newHeight)
    {
        auto calcWidthHeight = [](const int width, const int height, int& newWidth, int& newHeight)
        {
            if (width >= height)
            {
                newWidth = width > newWidth ? newWidth : width;
                newHeight = height > newHeight ? (newWidth * height / width) : height;
            }
            else
            {
                newHeight = height > newHeight ? newHeight : height;
                newWidth = width > newWidth ? (newHeight * width / height) : width;
            }
        };
        
        int width, height, channels, bytesPerChannel;
        void* pixels = nullptr;
        void* resizedPixels = nullptr;
        VkFormat format = VK_FORMAT_UNDEFINED;
        
        if (stbi_is_hdr(path.c_str()))
        {
            pixels = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            bytesPerChannel = 4;
            channels = 4;
            
            calcWidthHeight(width, height, newWidth, newHeight);
            
            resizedPixels = new float[channels * newWidth * newHeight];
            
            stbir_resize_float_linear((const float*)pixels, width, height, 0, (float*)resizedPixels, newWidth, newHeight, 0, stbir_pixel_layout::STBIR_RGBA);
        }
        else
        {
            pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            format = VK_FORMAT_R8G8B8A8_UNORM;
            bytesPerChannel = 1;
            channels = 4;
            
            calcWidthHeight(width, height, newWidth, newHeight);
            
            resizedPixels = new unsigned char[channels * newWidth * newHeight];
            
            stbir_resize_uint8_linear((const unsigned char*)pixels, width, height, 0, (unsigned char*)resizedPixels, newWidth, newHeight, 0, stbir_pixel_layout::STBIR_RGBA);
        }
        
        auto texture = std::make_shared<Texture2D>(resizedPixels, newWidth, newHeight, bytesPerChannel, format);
        
        if (bytesPerChannel == 4)
            delete [] (float*)resizedPixels;
        else
            delete [] (unsigned char*)resizedPixels;
        
        stbi_image_free(pixels);
        
        return texture;
    }

}
