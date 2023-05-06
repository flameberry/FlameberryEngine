#include "ContentBrowserPanel.h"

#include "Flameberry.h"
#include "../project.h"
#include "../Utils.h"

#define FL_BACK_ARROW_ICON 0
#define FL_FORWARD_ARROW_ICON 1
#define FL_FOLDER_ICON 2
#define FL_FILE_BERRY_ICON 3
#define FL_FILE_PNG_ICON 4
#define FL_FILE_JPG_ICON 5
#define FL_FILE_OBJ_ICON 6
#define FL_FILE_MTL_ICON 7
#define FL_FILE_JSON_ICON 8
#define FL_FILE_DEFAULT_ICON 9

static std::vector<std::string> g_IconPaths = {
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/arrow_back.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/arrow_forward.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/folder_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/file_berry_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/file_png_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/file_jpg_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/obj_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/mtl_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/json_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/file_icon_default.png"
};

namespace Flameberry {
    ContentBrowserPanel::ContentBrowserPanel()
        : m_CurrentDirectory(project::g_AssetDirectory),
        m_VkTextureSampler(VulkanTexture::GetDefaultSampler())
    {
        for (const auto& path : g_IconPaths)
            m_IconTextures.emplace_back(std::make_shared<VulkanTexture>(path.c_str(), m_VkTextureSampler));
    }

    ContentBrowserPanel::~ContentBrowserPanel()
    {
    }

    void ContentBrowserPanel::RecursivelyAddDirectoryNodes(const std::filesystem::directory_entry& parent, const std::filesystem::directory_iterator& iterator, bool open)
    {
        static bool is_selected = false;
        static const int treeNodeFlags = (is_selected ? ImGuiTreeNodeFlags_Selected : 0)
            | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanFullWidth
            | (open ? ImGuiTreeNodeFlags_DefaultOpen : 0);

        ImGui::PushID(parent.path().filename().c_str());
        bool is_opened = ImGui::TreeNodeEx("##node", treeNodeFlags);
        if (ImGui::IsItemClicked())
        {
            m_CurrentDirectory = parent.path();
            is_selected = true;
        }
        ImGui::SameLine();
        ImGui::Image(reinterpret_cast<ImTextureID>(m_IconTextures[FL_FOLDER_ICON]->GetDescriptorSet()), ImVec2{ ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() });
        ImGui::SameLine();
        ImGui::Text("%s", parent.path().filename().c_str());
        ImGui::PopID();
        if (is_opened) {
            for (auto& directory : iterator) {
                if (directory.is_directory()) {
                    RecursivelyAddDirectoryNodes(directory, std::filesystem::directory_iterator{ directory });
                }
            }
            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Content Browser");
        ImGui::PopStyleVar();

        m_SecondChildSize = ImGui::GetWindowContentRegionMax().x - m_FirstChildSize;

        Utils::Splitter(true, 3.0f, &m_FirstChildSize, &m_SecondChildSize, 10.0f, 80.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 7 });
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);
        ImGui::BeginChild("##FileStructurePanel", ImVec2(m_FirstChildSize, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);

        for (auto& directory : std::filesystem::directory_iterator(project::g_AssetDirectory)) {
            if (directory.is_directory()) {
                RecursivelyAddDirectoryNodes(directory, std::filesystem::directory_iterator(directory), false);
            }
        }

        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::PopStyleVar();

        ImGui::BeginChild("##Contents", ImVec2(m_SecondChildSize, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleVar();

        float iconSize = 80.0f, padding = 10.0f;
        float cellSize = iconSize + padding;
        uint32_t columns = ImGui::GetContentRegionAvail().x / cellSize;
        columns = columns >= 1 ? columns : 1;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

        float arrowSize = 18.0f;

        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FL_BACK_ARROW_ICON]->GetDescriptorSet()), ImVec2{ arrowSize, arrowSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }) && m_CurrentDirectory != project::g_AssetDirectory)
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FL_FORWARD_ARROW_ICON]->GetDescriptorSet()), ImVec2{ arrowSize, arrowSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }) && m_CurrentDirectory != project::g_AssetDirectory)
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
            int currentIconIndex;
            bool is_file_supported = true;

            if (directory.is_directory())
                currentIconIndex = FL_FOLDER_ICON;
            else if (ext == ".berry")
                currentIconIndex = FL_FILE_BERRY_ICON;
            else if (ext == ".png")
                currentIconIndex = FL_FILE_PNG_ICON;
            else if (ext == ".jpg")
                currentIconIndex = FL_FILE_JPG_ICON;
            else if (ext == ".obj")
                currentIconIndex = FL_FILE_OBJ_ICON;
            else if (ext == ".mtl")
                currentIconIndex = FL_FILE_MTL_ICON;
            else if (ext == ".json")
                currentIconIndex = FL_FILE_JSON_ICON;
            else {
                currentIconIndex = FL_FILE_DEFAULT_ICON;
                is_file_supported = false;
            }

            ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[currentIconIndex]->GetDescriptorSet()), ImVec2{ iconSize, iconSize });

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

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Open In Finder"))
                platform::OpenInFinder(m_CurrentDirectory.string().c_str());
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
        ImGui::EndChild();

        ImGui::End();
    }
}
