#include "ThumbnailCache.h"

#include "Asset/Importers/TextureImporter.h"

namespace Flameberry {

	ThumbnailCache::ThumbnailCache(const std::filesystem::path& thumbnailCacheDirectory, const ThumbnailCacheConfig& config)
		: m_ThumbnailCacheDirectory(thumbnailCacheDirectory), m_Config(config)
	{
	}

	Ref<Texture2D> ThumbnailCache::GetOrCreateThumbnail(const std::filesystem::path& assetPath)
	{
		const std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(assetPath);
		const uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(lastWriteTime.time_since_epoch()).count();

		if (m_CachedThumbnails.find(assetPath) != m_CachedThumbnails.end())
		{
			const auto& cachedThumbnail = m_CachedThumbnails.at(assetPath);
			if (cachedThumbnail.Timestamp == timestamp)
				return cachedThumbnail.Image;
		}

		// TODO: Expand the number of extensions
		if (m_ThumbnailsLoadedThisFrame >= m_Config.MaxThumbnailsLoadedPerFrame || (assetPath.extension() != ".png" && assetPath.extension() != ".jpg" && assetPath.extension() != ".hdr" && assetPath.extension() != ".tga"))
			return nullptr;

		const auto thumbnail = std::static_pointer_cast<Texture2D>(TextureImporter::LoadTexture2DResized(assetPath, 128, 128, false));
		auto& cachedThumbnail = m_CachedThumbnails[assetPath];
		cachedThumbnail.Timestamp = timestamp;
		cachedThumbnail.Image = thumbnail;

		m_ThumbnailsLoadedThisFrame++;
		return cachedThumbnail.Image;
	}

} // namespace Flameberry
