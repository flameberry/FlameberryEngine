#pragma once

#include <string>
#include <filesystem>

#include "Flameberry.h"

namespace Flameberry {
    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel(const std::filesystem::path& projectDirectory);
        ~ContentBrowserPanel();
        void OnUIRender();
    private:
        void RecursivelyAddDirectoryNodes(const std::filesystem::directory_entry& parent, const std::filesystem::directory_iterator& iterator);
    private:
        std::filesystem::path m_CurrentDirectory, m_ProjectDirectory;

        VkSampler m_VkTextureSampler;
        std::vector<std::shared_ptr<VulkanTexture>> m_IconTextures;

        float m_FirstChildSize = 150.0f, m_SecondChildSize = 0.0f;
        bool m_IsSelectedNodeDisplayed = false, m_IsSearchBoxFocused = false;
    };
}