#pragma once

#include <string>
#include <filesystem>

namespace Flameberry {
    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel();
        ~ContentBrowserPanel();
        void OnUIRender();
    private:
        std::filesystem::path m_CurrentDirectory;
        uint32_t m_BackArrowIconTextureId = 0;
    };
}