#include "ContentBrowserPanel.h"

#include "Flameberry.h"
#include "../Utils.h"

#define FL_BACK_ARROW_ICON 0
#define FL_FORWARD_ARROW_ICON 1

enum FileTypeIndex {
    DEFAULT = 2,
    FOLDER, BERRY, PNG, JPG, OBJ, FBMAT, MTL, JSON, HDR
};

static std::vector<std::string> g_IconPaths = {
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/arrow_back.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/arrow_forward.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/default_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/folder_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/berry_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/png_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/jpg_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/obj_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/fbmat_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/mtl_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/json_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/assets/icons/hdr_file_icon.png"
};

namespace Flameberry {
    ContentBrowserPanel::ContentBrowserPanel(const std::filesystem::path& projectDirectory)
        : m_ProjectDirectory(projectDirectory),
        m_CurrentDirectory("Assets"),
        m_VkTextureSampler(Texture2D::GetDefaultSampler())
    {
        for (const auto& path : g_IconPaths)
            m_IconTextures.emplace_back(std::make_shared<Texture2D>(path.c_str(), m_VkTextureSampler));
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
        const bool is_selected = m_CurrentDirectory == parent;

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

        if (IsPathInHierarchy(m_CurrentDirectory, parent)) {
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
        ImGui::Image(reinterpret_cast<ImTextureID>(m_IconTextures[FileTypeIndex::FOLDER]->GetDescriptorSet()), ImVec2{ ImGui::GetTextLineHeight() + framePaddingY, ImGui::GetTextLineHeight() + framePaddingY });
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

    bool ContentBrowserPanel::IsPathInHierarchy(const std::filesystem::path& key, const std::filesystem::path& parent)
    {
        const auto& keystr = key.string();
        const auto& parentstr = parent.string();

        if (keystr.size() <= parentstr.size())
            return false;

        for (int i = parentstr.size() - 1; i >= 0; i--)
        {
            if (keystr[i] != parentstr[i])
                return false;
        }
        return true;
    }

    void ContentBrowserPanel::OnUIRender()
    {
        FL_ASSERT(std::filesystem::exists(m_ProjectDirectory / "Assets"), "Failed to find Assets directory!");

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Content Browser");
        ImGui::PopStyleVar();

        m_SecondChildSize = ImGui::GetWindowContentRegionWidth() - m_FirstChildSize - 8.0f;

        Utils::Splitter(true, 3.0f, &m_FirstChildSize, &m_SecondChildSize, 10.0f, 80.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 7 });
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);
        ImGui::BeginChild("##FileStructurePanel", ImVec2(m_FirstChildSize, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);

        for (auto& directory : std::filesystem::directory_iterator("Assets")) {
            if (directory.is_directory()) {
                RecursivelyAddDirectoryNodes(directory, std::filesystem::directory_iterator(directory));
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

        float topChildHeight = 38.0f;
        float bottomChildHeight = ImGui::GetContentRegionAvail().y - topChildHeight;

        ImGui::BeginChild("##ContentBrowserTopBar", ImVec2(m_SecondChildSize, topChildHeight), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        float arrowSize = 18.0f;
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FL_BACK_ARROW_ICON]->GetDescriptorSet()), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Assets")
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FL_FORWARD_ARROW_ICON]->GetDescriptorSet()), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Assets")
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();

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
        ImGui::Text("%s", m_CurrentDirectory.c_str());
        ImGui::PopFont();

        ImVec2 pos = ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowHeight() + ImGui::GetWindowPos().y);
        ImGui::GetWindowDrawList()->AddLine(pos, ImVec2{ pos.x + ImGui::GetWindowWidth() - 5.0f, pos.y }, 0xff646161, 4.0f);
        ImGui::EndChild();

        ImGui::SetNextWindowPos(pos);

        ImGui::BeginChild("##Contents", ImVec2(m_SecondChildSize, bottomChildHeight), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleVar();

        float iconWidth = 75.0f, padding = 15.0f;

        float cellSize = iconWidth + padding;
        uint32_t columns = ImGui::GetContentRegionAvail().x / cellSize;
        columns = columns >= 1 ? columns : 1;
        ImGui::Columns(columns, (const char*)__null, false);

        for (const auto& directory : std::filesystem::directory_iterator{ m_CurrentDirectory })
        {
            const std::filesystem::path& filePath = directory.path();

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
                currentIconIndex = FileTypeIndex::FOLDER;
            else if (ext == ".berry")
                currentIconIndex = FileTypeIndex::BERRY;
            else if (ext == ".png")
                currentIconIndex = FileTypeIndex::PNG;
            else if (ext == ".jpg")
                currentIconIndex = FileTypeIndex::JPG;
            else if (ext == ".obj")
                currentIconIndex = FileTypeIndex::OBJ;
            else if (ext == ".mtl")
                currentIconIndex = FileTypeIndex::MTL;
            else if (ext == ".fbmat")
                currentIconIndex = FileTypeIndex::FBMAT;
            else if (ext == ".json")
                currentIconIndex = FileTypeIndex::JSON;
            else if (ext == ".hdr")
                currentIconIndex = FileTypeIndex::HDR;
            else {
                currentIconIndex = FileTypeIndex::DEFAULT;
                is_file_supported = false;
            }

            // TODO: Precalculate iconWidths for all
            // float iconHeight = iconWidth * m_IconTextures[currentIconIndex]->GetImageSpecification().Height / m_IconTextures[currentIconIndex]->GetImageSpecification().Width;

            ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[currentIconIndex]->GetDescriptorSet()), ImVec2{ iconWidth, iconWidth });

            const auto& filename = is_file_supported ? directory.path().stem() : directory.path().filename();

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
                    filePath.c_str(),
                    (strlen(filePath.c_str()) + 1) * sizeof(char),
                    ImGuiCond_Once
                );
                ImGui::Text("%s", filePath.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && directory.is_directory())
                m_CurrentDirectory = directory.path();

            auto columnWidth = ImGui::GetColumnWidth();
            auto textWidth = ImGui::CalcTextSize(filename.c_str()).x;
            const auto aSize = ImGui::CalcTextSize("A");

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 5.0f, 20.0f });
            if (textWidth > columnWidth) {
                uint32_t characters = columnWidth / aSize.x - 3;
                ImGui::Text("%.*s%s", characters, filename.c_str(), "...");
            }
            else {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f - ImGui::GetScrollX() - 1 * ImGui::GetStyle().ItemSpacing.x);
                ImGui::Text("%s", filename.c_str());
            }
            ImGui::PopStyleVar();

            ImGui::NextColumn();
            ImGui::PopID();
        }

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Material"))
                {
                    auto mat = std::make_shared<Material>();
                    MaterialSerializer::Serialize(mat, (m_CurrentDirectory / "NewMaterial.fbmat").c_str());
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Open In Finder"))
                platform::OpenInFinder((m_ProjectDirectory / m_CurrentDirectory).string().c_str());
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::EndChild();

        ImGui::End();
    }
}
