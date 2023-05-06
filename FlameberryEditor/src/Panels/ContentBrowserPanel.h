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
        void RecursivelyAddDirectoryNodes(const std::filesystem::directory_entry& parent, const std::filesystem::directory_iterator& iterator, bool open = false);
    private:
        std::filesystem::path m_CurrentDirectory;

        VkSampler m_VkTextureSampler;
        std::vector<std::shared_ptr<VulkanTexture>> m_IconTextures;

        float m_FirstChildSize = 150.0f, m_SecondChildSize = 0.0f;
    };
}