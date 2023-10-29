#pragma once

#include <map>
#include <filesystem>

// TODO: Make the paths unambigious
#include "Project/Project.h"
#include "Renderer/Texture2D.h"

namespace Flameberry {
    
    struct Thumbnail
    {
        uint64_t Timestamp;
        std::shared_ptr<Texture2D> Image;
    };
    
    struct ThumbnailCacheConfig
    {
        uint16_t MaxThumbnailsLoadedPerFrame = 1;
    };
    
    class ThumbnailCache
    {
    public:
        ThumbnailCache(const std::shared_ptr<Project>& project, const ThumbnailCacheConfig& config = ThumbnailCacheConfig());
        
        std::shared_ptr<Texture2D> TryGetOrCreateThumbnail(const std::filesystem::path& assetPath);
        void ResetThumbnailLoadedCounter() { m_ThumbnailsLoadedThisFrame = 0; }
    private:
        std::shared_ptr<Project> m_Project;
        std::filesystem::path m_ThumbnailCacheDirectory;
        ThumbnailCacheConfig m_Config;
        
        uint16_t m_ThumbnailsLoadedThisFrame = 0;
        
        std::map<std::filesystem::path, Thumbnail> m_CachedThumbnails;
    };

}
