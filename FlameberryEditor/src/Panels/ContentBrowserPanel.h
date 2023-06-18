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

        template<typename... Args>
        static std::shared_ptr<ContentBrowserPanel> Create(Args... args) { return std::make_shared<ContentBrowserPanel>(std::forward<Args>(args)...); }
    private:
        void RecursivelyAddDirectoryNodes(const std::filesystem::directory_entry& parent, const std::filesystem::directory_iterator& iterator);
        bool IsPathInHierarchy(const std::filesystem::path& key, const std::filesystem::path& parent);
    private:
        std::filesystem::path m_CurrentDirectory, m_ProjectDirectory;

        VkSampler m_VkTextureSampler;
        std::vector<std::shared_ptr<VulkanTexture>> m_IconTextures;

        float m_FirstChildSize = 150.0f, m_SecondChildSize = 0.0f;
        bool m_IsSearchBoxFocused = false;
        char m_SearchInputBuffer[256] = { '\0' };
    };
}