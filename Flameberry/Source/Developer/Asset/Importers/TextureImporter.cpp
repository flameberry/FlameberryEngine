#include "TextureImporter.h"
#include "Renderer/Texture2D.h"

#include <stb_image/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image/stb_image_resize2.h>

namespace Flameberry {

	Ref<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadTexture2D(metadata.FilePath);
	}

	Ref<Texture2D> TextureImporter::LoadTexture2D(const std::filesystem::path& path)
	{
		int width, height, channels;
		void* pixels = nullptr;
		VkFormat format = VK_FORMAT_UNDEFINED;

		if (stbi_is_hdr(path.c_str()))
		{
			pixels = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		else
		{
			pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			format = VK_FORMAT_R8G8B8A8_UNORM;
		}

		Texture2DSpecification specification;
		specification.Width = width;
		specification.Height = height;
		specification.Format = format;

		Ref<Texture2D> asset = CreateRef<Texture2D>(pixels, specification);

		stbi_image_free(pixels);

		return asset;
	}

	// TODO: Needs work quality wise
	Ref<Texture2D> TextureImporter::LoadTexture2DResized(const std::filesystem::path& path, int newWidth, int newHeight, bool bGenerateMipmaps)
	{
		auto calcWidthHeight = [](const int width, const int height, int& newWidth, int& newHeight) {
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

		int width, height, channels;
		void* pixels = nullptr;
		void* resizedPixels = nullptr;
		VkFormat format = VK_FORMAT_UNDEFINED;

		if (stbi_is_hdr(path.c_str()))
		{
			pixels = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
			channels = 4;

			calcWidthHeight(width, height, newWidth, newHeight);

			resizedPixels = new float[channels * newWidth * newHeight];

			stbir_resize_float_linear((const float*)pixels, width, height, 0, (float*)resizedPixels, newWidth, newHeight, 0, stbir_pixel_layout::STBIR_RGBA);
		}
		else
		{
			pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			format = VK_FORMAT_R8G8B8A8_UNORM;
			channels = 4;

			calcWidthHeight(width, height, newWidth, newHeight);

			resizedPixels = new unsigned char[channels * newWidth * newHeight];

			stbir_resize_uint8_linear((const unsigned char*)pixels, width, height, 0, (unsigned char*)resizedPixels, newWidth, newHeight, 0, stbir_pixel_layout::STBIR_RGBA);
		}

		Texture2DSpecification specification;
		specification.Width = newWidth;
		specification.Height = newHeight;
		specification.Format = format;
		specification.GenerateMipmaps = bGenerateMipmaps;

		auto texture = CreateRef<Texture2D>(resizedPixels, specification);

		if (format == VK_FORMAT_R32G32B32A32_SFLOAT)
			delete[] (float*)resizedPixels;
		else
			delete[] (unsigned char*)resizedPixels;

		stbi_image_free(pixels);

		return texture;
	}

} // namespace Flameberry
