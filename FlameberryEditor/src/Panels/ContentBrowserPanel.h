#pragma once

#include <string>
#include <filesystem>

#include "Flameberry.h"

#include "ThumbnailCache.h"

namespace Flameberry {
    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel();
        ~ContentBrowserPanel();
        void OnUIRender();
    private:
        void RecursivelyAddDirectoryNodes(const std::filesystem::directory_entry& parent, const std::filesystem::directory_iterator& iterator);
        bool IsPathInHierarchy(const std::filesystem::path& key, const std::filesystem::path& parent);

        void UI_CurrentPathBar();
    private:
        std::filesystem::path m_CurrentDirectory;

        VkSampler m_VkTextureSampler;
        std::vector<Ref<Texture2D>> m_IconTextures;

        Ref<ThumbnailCache> m_ThumbnailCache;

        static constexpr ImGuiPopupFlags m_PopupFlags = ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;

        float m_FirstChildSize = 170.0f, m_SecondChildSize = 0.0f;
        uint32_t m_ThumbnailSize = 110;
        bool m_IsSearchBoxFocused = false;
        char m_SearchInputBuffer[256] = { '\0' };
    };
}
