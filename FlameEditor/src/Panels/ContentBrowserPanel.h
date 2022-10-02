#pragma once

#include <string>
#include <filesystem>

#include "Panel.h"

class ContentBrowserPanel : public Panel
{
public:
    ContentBrowserPanel();
    virtual ~ContentBrowserPanel();

    void OnUIRender() override;
public:
    static std::filesystem::path s_SourceDirectory;
private:
    std::filesystem::path m_CurrentDirectory;
private:
    uint32_t m_BackArrowIconTextureId = 0;
};
