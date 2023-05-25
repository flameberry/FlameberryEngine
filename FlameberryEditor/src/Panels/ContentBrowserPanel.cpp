#include "ContentBrowserPanel.h"

#include "Flameberry.h"
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
    ContentBrowserPanel::ContentBrowserPanel(const std::filesystem::path& projectDirectory)
        : m_ProjectDirectory(projectDirectory),
        m_CurrentDirectory(projectDirectory / "Assets"),
        m_VkTextureSampler(VulkanTexture::GetDefaultSampler())
    {
        for (const auto& path : g_IconPaths)
            m_IconTextures.emplace_back(std::make_shared<VulkanTexture>(path.c_str(), m_VkTextureSampler));
    }

    ContentBrowserPanel::~ContentBrowserPanel()
    {
    }

    void ContentBrowserPanel::RecursivelyAddDirectoryNodes(const std::filesystem::directory_entry& parent, const std::filesystem::directory_iterator& iterator)
    {
        bool is_leaf = true;
        for (auto& directory : std::filesystem::directory_iterator{ parent }) {
            is_leaf = is_leaf && !directory.is_directory();
        }
        const bool is_selected = m_CurrentDirectory == parent.path();
        m_IsSelectedNodeDisplayed = m_IsSelectedNodeDisplayed || is_selected;

        const int treeNodeFlags = (is_selected ? ImGuiTreeNodeFlags_Selected : 0)
            | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanFullWidth
            | ImGuiTreeNodeFlags_FramePadding
            | (is_leaf ? ImGuiTreeNodeFlags_Leaf : 0);

        ImGui::PushID(parent.path().filename().c_str());

        float textColor = is_selected ? 0.0f : 1.0f;
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{ 1.0f, 197.0f / 255.0f, 86.0f / 255.0f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
        if (is_selected)
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ textColor, textColor, textColor, 1.0f });

        if (!m_IsSelectedNodeDisplayed) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        }

        constexpr float framePaddingY = 1.8f;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, framePaddingY });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
        bool is_opened = ImGui::TreeNodeEx("##node", treeNodeFlags);
        ImGui::PopStyleVar(2);

        if (ImGui::IsItemClicked())
            m_CurrentDirectory = parent.path();

        ImGui::SameLine();
        ImGui::Image(reinterpret_cast<ImTextureID>(m_IconTextures[FL_FOLDER_ICON]->GetDescriptorSet()), ImVec2{ ImGui::GetTextLineHeight() + framePaddingY, ImGui::GetTextLineHeight() + framePaddingY });
        ImGui::SameLine();
        ImGui::Text("%s", parent.path().filename().c_str());
        ImGui::PopStyleColor(is_selected ? 4 : 3);
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

        m_SecondChildSize = ImGui::GetWindowContentRegionWidth() - m_FirstChildSize - 8.0f;

        Utils::Splitter(true, 3.0f, &m_FirstChildSize, &m_SecondChildSize, 10.0f, 80.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 7 });
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);
        ImGui::BeginChild("##FileStructurePanel", ImVec2(m_FirstChildSize, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);

        m_IsSelectedNodeDisplayed = false;
        for (auto& directory : std::filesystem::directory_iterator(m_ProjectDirectory / "Assets")) {
            if (directory.is_directory()) {
                RecursivelyAddDirectoryNodes(directory, std::filesystem::directory_iterator(directory));
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::SameLine();

        ImGui::BeginChild("##Contents", ImVec2(m_SecondChildSize, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleVar();

        float iconSize = 75.0f, padding = 15.0f;
        float cellSize = iconSize + padding;
        uint32_t columns = ImGui::GetContentRegionAvail().x / cellSize;
        columns = columns >= 1 ? columns : 1;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

        float arrowSize = 18.0f;

        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FL_BACK_ARROW_ICON]->GetDescriptorSet()), ImVec2{ arrowSize, arrowSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }) && m_CurrentDirectory != m_ProjectDirectory / "Assets")
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FL_FORWARD_ARROW_ICON]->GetDescriptorSet()), ImVec2{ arrowSize, arrowSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }) && m_CurrentDirectory != m_ProjectDirectory / "Assets")
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();

        // char search_input[256] = { '\0' };
        ImGui::PushItemWidth(150.0f);
        if (m_IsSearchBoxFocused)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
        }

        ImGui::InputTextWithHint("##search", "Search...", m_SearchInputBuffer, 256);

        if (m_IsSearchBoxFocused)
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        ImGui::PopItemWidth();
        m_IsSearchBoxFocused = ImGui::IsItemActive() && ImGui::IsItemFocused();

        ImGui::SameLine();

        auto bigFont = ImGui::GetIO().Fonts->Fonts[0];
        ImGui::PushFont(bigFont);
        ImGui::Text("%s", std::filesystem::relative(m_CurrentDirectory, m_ProjectDirectory).c_str());
        ImGui::PopFont();

        ImGui::Separator();

        ImGui::Columns(columns, (const char*)__null, false);

        for (const auto& directory : std::filesystem::directory_iterator{ m_CurrentDirectory })
        {
            const std::filesystem::path& filePath = directory.path();
            std::filesystem::path relativePath = std::filesystem::relative(directory.path(), m_ProjectDirectory);

            if (filePath.filename().string() == ".DS_Store")
                continue;

            if (m_SearchInputBuffer[0] != '\0') {
                // TODO: Maybe some optimisation to not search again if the input string is same
                int index = kmpSearch(filePath.filename().replace_extension().c_str(), m_SearchInputBuffer, true);
                if (index == -1)
                    continue;
            }

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

            const auto& filename_noextension = is_file_supported ? directory.path().filename().replace_extension() : directory.path().filename();

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
                m_CurrentDirectory = directory.path();

            auto columnWidth = ImGui::GetColumnWidth();
            auto textWidth = ImGui::CalcTextSize(filename_noextension.c_str()).x;
            const auto aSize = ImGui::CalcTextSize("A");

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 5.0f, 20.0f });
            if (textWidth > columnWidth) {
                uint32_t characters = columnWidth / aSize.x - 3;
                ImGui::Text("%.*s%s", characters, filename_noextension.c_str(), "...");
            }
            else {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f - ImGui::GetScrollX() - 1 * ImGui::GetStyle().ItemSpacing.x);
                ImGui::Text("%s", filename_noextension.c_str());
            }
            ImGui::PopStyleVar();

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

        ImGui::ShowDemoWindow();
    }
}
