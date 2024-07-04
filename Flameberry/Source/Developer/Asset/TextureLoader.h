#pragma once

#include "Renderer/Texture2D.h"

namespace Flameberry {
	class TextureLoader
	{
	public:
		static Ref<Asset> LoadTexture2D(const std::filesystem::path& path);

		// Takes in newWidth and newHeight, also can be called as the maxWidth and maxHeight for the thumbnail, as the aspect ratio of the image will be maintained
		static Ref<Texture2D> LoadTexture2DResized(const std::filesystem::path& path, int newWidth, int newHeight);
	};
} // namespace Flameberry
