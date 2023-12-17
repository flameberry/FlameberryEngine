#include "ThumbnailCache.h"

#include "Asset/TextureLoader.h"

namespace Flameberry {
    
    ThumbnailCache::ThumbnailCache(const Ref<Project>& project, const ThumbnailCacheConfig& config)
        : m_Project(project), m_ThumbnailCacheDirectory(m_Project->GetProjectDirectory() / "Thumbnail.cache"), m_Config(config)
    {
    }
    
    Ref<Texture2D> ThumbnailCache::TryGetOrCreateThumbnail(const std::filesystem::path& assetPath)
    {
        auto absolutePath = m_Project->GetProjectDirectory() / assetPath;
        std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(absolutePath);
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(lastWriteTime.time_since_epoch()).count();
        
        if (m_CachedThumbnails.find(assetPath) != m_CachedThumbnails.end())
        {
            auto& cachedThumbnail = m_CachedThumbnails.at(assetPath);
            if (cachedThumbnail.Timestamp == timestamp)
                return cachedThumbnail.Image;
        }
        
        // TODO: Expand the number of extensions
        if (m_ThumbnailsLoadedThisFrame >= m_Config.MaxThumbnailsLoadedPerFrame || (assetPath.extension() != ".png" && assetPath.extension() != ".jpg" && assetPath.extension() != ".hdr" && assetPath.extension() != ".tga"))
            return nullptr;
        
        auto thumbnail = std::static_pointer_cast<Texture2D>(TextureLoader::LoadTexture2DResized(absolutePath, 128, 128));
        auto& cachedThumbnail = m_CachedThumbnails[assetPath];
        cachedThumbnail.Timestamp = timestamp;
        cachedThumbnail.Image = thumbnail;
        
        m_ThumbnailsLoadedThisFrame++;
        return cachedThumbnail.Image;
    }
    
}
