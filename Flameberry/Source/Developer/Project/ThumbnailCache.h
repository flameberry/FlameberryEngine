#pragma once

#include <map>
#include <filesystem>

#include "Renderer/Texture2D.h"

namespace Flameberry {

	struct Thumbnail
	{
		uint64_t Timestamp;
		Ref<Texture2D> Image;
	};

	struct ThumbnailCacheConfig
	{
		uint16_t MaxThumbnailsLoadedPerFrame = 1;
	};

	class ThumbnailCache
	{
	public:
		ThumbnailCache(const std::filesystem::path& thumbnailCacheDirectory, const ThumbnailCacheConfig& config = ThumbnailCacheConfig());

		Ref<Texture2D> GetOrCreateThumbnail(const std::filesystem::path& assetPath);
		void ResetThumbnailLoadedCounter() { m_ThumbnailsLoadedThisFrame = 0; }

	private:
		std::filesystem::path m_ThumbnailCacheDirectory;
		ThumbnailCacheConfig m_Config;

		uint16_t m_ThumbnailsLoadedThisFrame = 0;

		std::map<std::filesystem::path, Thumbnail> m_CachedThumbnails;
	};

} // namespace Flameberry
