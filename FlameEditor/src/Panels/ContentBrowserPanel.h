#pragma once

#include <string>
#include <filesystem>

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
