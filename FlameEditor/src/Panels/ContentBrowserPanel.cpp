#include "ContentBrowserPanel.h"

#include "Flameberry.h"

std::filesystem::path ContentBrowserPanel::s_SourceDirectory{ FL_PROJECT_DIR"SandboxApp/assets" };

ContentBrowserPanel::ContentBrowserPanel()
    : m_CurrentDirectory(s_SourceDirectory)
{
    m_BackArrowIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/back_arrow_icon.png");
}

ContentBrowserPanel::~ContentBrowserPanel()
{
}

void ContentBrowserPanel::OnUIRender()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 10, 7 });
    ImGui::Begin("Content Browser");
    ImGui::PopStyleVar();

    float iconSize = 80.0f, padding = 10.0f;
    float cellSize = iconSize + padding;
    uint32_t columns = ImGui::GetContentRegionAvail().x / cellSize;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_BackArrowIconTextureId), ImVec2{ 15, 15 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }) && m_CurrentDirectory != s_SourceDirectory)
        m_CurrentDirectory = m_CurrentDirectory.parent_path();
    ImGui::Separator();

    ImGui::Columns(columns, (const char*)__null, false);

    for (const auto& directory : std::filesystem::directory_iterator{ m_CurrentDirectory })
    {
        const std::filesystem::path& filePath = directory.path();
        std::filesystem::path relativePath = std::filesystem::relative(directory.path(), s_SourceDirectory);

        ImGui::PushID(filePath.filename().c_str());
        std::string ext = filePath.extension().string();
        uint32_t currentIconTextureId;

        if (directory.is_directory())
            currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/folder_icon_white.png");
        else if (ext == ".png")
            currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/file_png_icon.png");
        else if (ext == ".jpg")
            currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/file_jpg_icon.png");
        else
            currentIconTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/file_icon_default.png");

        ImGui::ImageButton(reinterpret_cast<ImTextureID>(currentIconTextureId), ImVec2{ 80, 80 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(
                "FL_FILE_PATH",
                relativePath.c_str(),
                (strlen(relativePath.c_str()) + 1) * sizeof(char),
                ImGuiCond_Once
            );
            ImGui::Text("%s", relativePath.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && directory.is_directory())
            m_CurrentDirectory /= directory.path().filename().c_str();
        ImGui::TextWrapped("%s", directory.path().filename().c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();


    ImGui::End();
}
