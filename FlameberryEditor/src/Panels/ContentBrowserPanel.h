#pragma once

#include <string>
#include <filesystem>

#include "Flameberry.h"

namespace Flameberry {
    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel();
        ~ContentBrowserPanel();
        void OnUIRender();
    private:
        std::filesystem::path m_CurrentDirectory;

        VkSampler m_VkTextureSampler;
        std::vector<std::shared_ptr<VulkanTexture>> m_IconTextures;
    };
}