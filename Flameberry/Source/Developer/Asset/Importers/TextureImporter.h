#pragma once

#include "Asset/AssetMetadata.h"
#include "Renderer/Texture2D.h"

namespace Flameberry {

	class TextureImporter
	{
	public:
		static Ref<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata);
		static Ref<Texture2D> LoadTexture2D(const std::filesystem::path& path);

		// Takes in newWidth and newHeight, also can be called as the maxWidth and maxHeight for the thumbnail, as the aspect ratio of the image will be maintained
		static Ref<Texture2D> LoadTexture2DResized(const std::filesystem::path& path, int newWidth, int newHeight, bool bGenerateMipmaps);
	};

} // namespace Flameberry
