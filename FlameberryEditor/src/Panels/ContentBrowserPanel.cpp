#include "ContentBrowserPanel.h"

#include "Flameberry.h"
#include "../project.h"

namespace Flameberry {
    ContentBrowserPanel::ContentBrowserPanel()
        : m_CurrentDirectory(project::g_AssetDirectory)
    {
        m_BackArrowIconTextureID = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/arrow_back.png");
        m_ForwardArrowIconTextureID = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/arrow_forward.png");
    }

    ContentBrowserPanel::~ContentBrowserPanel()
    {
    }

    void ContentBrowserPanel::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 10, 7 });
        ImGui::Begin("Content Browser");
        ImGui::PopStyleVar();

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Open In Finder"))
                Platform::OpenInFinder(m_CurrentDirectory.string().c_str());
            ImGui::EndPopup();
        }

        float iconSize = 80.0f, padding = 10.0f;
        float cellSize = iconSize + padding;
        uint32_t columns = ImGui::GetContentRegionAvail().x / cellSize;
        columns = columns >= 1 ? columns : 1;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

        float arrowSize = 18.0f;

        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_BackArrowIconTextureID), ImVec2{ arrowSize, arrowSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }) && m_CurrentDirectory != project::g_AssetDirectory)
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_ForwardArrowIconTextureID), ImVec2{ arrowSize, arrowSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }) && m_CurrentDirectory != project::g_AssetDirectory)
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();

        char search_input[256] = { '\0' };
        ImGui::PushItemWidth(150.0f);
        ImGui::InputTextWithHint("##search", "search", search_input, 256);
        bool isSearchBoxActive = ImGui::IsItemActive() && ImGui::IsItemFocused();
        ImGui::PopItemWidth();

        ImGui::SameLine();

        auto bigFont = ImGui::GetIO().Fonts->Fonts[0];
        ImGui::PushFont(bigFont);
        ImGui::Text("Assets / %s", std::filesystem::relative(m_CurrentDirectory, project::g_AssetDirectory).c_str());
        ImGui::PopFont();

        ImGui::Separator();

        ImGui::Columns(columns, (const char*)__null, false);

        for (const auto& directory : std::filesystem::directory_iterator{ m_CurrentDirectory })
        {
            const std::filesystem::path& filePath = directory.path();
            std::filesystem::path relativePath = std::filesystem::relative(directory.path(), project::g_AssetDirectory);

            if (filePath.filename().string() == ".DS_Store")
                continue;

            ImGui::PushID(filePath.filename().c_str());
            std::string ext = filePath.extension().string();
            uint32_t currentIconTextureId;
            bool is_file_supported = true;

            if (directory.is_directory())
                currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/folder_icon.png");
            else if (ext == ".berry")
                currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/file_berry_icon.png");
            else if (ext == ".png")
                currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/file_png_icon.png");
            else if (ext == ".jpg")
                currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/file_jpg_icon.png");
            else if (ext == ".obj")
                currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/obj_file_icon.png");
            else if (ext == ".mtl")
                currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/mtl_file_icon.png");
            else if (ext == ".json")
                currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/json_file_icon.png");
            else {
                currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameberryEditor/icons/file_icon_default.png");
                is_file_supported = false;
            }

            ImGui::ImageButton(reinterpret_cast<ImTextureID>(currentIconTextureId), ImVec2{ iconSize, iconSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                    FL_LOG("Delete");
                ImGui::EndMenu();
            }

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload(
                    "FL_CONTENT_BROWSER_ITEM",
                    relativePath.c_str(),
                    (strlen(relativePath.c_str()) + 1) * sizeof(char),
                    ImGuiCond_Once
                );
                ImGui::Text("%s", relativePath.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && directory.is_directory())
                m_CurrentDirectory /= directory.path().filename().c_str();

            auto columnWidth = ImGui::GetColumnWidth();
            const auto& filename_noextension = is_file_supported ? directory.path().filename().replace_extension() : directory.path().filename();
            auto textWidth = ImGui::CalcTextSize(filename_noextension.c_str()).x;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f - ImGui::GetScrollX() - 1 * ImGui::GetStyle().ItemSpacing.x);
            uint32_t limit = 10;
            ImGui::TextWrapped("%.*s%s", limit, filename_noextension.c_str(), filename_noextension.string().length() > limit - 2 ? "..." : "");

            ImGui::NextColumn();
            ImGui::PopID();
        }
        ImGui::PopStyleColor(1);
        ImGui::End();
    }
}
