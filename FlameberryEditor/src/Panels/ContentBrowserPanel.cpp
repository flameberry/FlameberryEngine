#include "ContentBrowserPanel.h"

#include "Flameberry.h"
#include "../UI.h"

#define FL_BACK_ARROW_ICON 0
#define FL_FORWARD_ARROW_ICON 1

enum FileTypeIndex {
    DEFAULT = 2,
    FOLDER, BERRY, PNG, JPG, OBJ, FBMAT, MTL, JSON, HDR
};

static std::vector<std::string> g_IconPaths = {
    FL_PROJECT_DIR"FlameberryEditor/icons/arrow_back.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/arrow_forward.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/default_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/folder_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/berry_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/png_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/jpg_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/obj_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/fbmat_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/mtl_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/json_file_icon.png",
    FL_PROJECT_DIR"FlameberryEditor/icons/hdr_file_icon.png"
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
        ImGui::PushStyleColor(ImGuiCol_Header, Theme::AccentColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, Theme::AccentColorLight);
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
        ImGui::Image(reinterpret_cast<ImTextureID>(m_IconTextures[FileTypeIndex::FOLDER]->CreateOrGetDescriptorSet()), ImVec2{ ImGui::GetTextLineHeight() + framePaddingY, ImGui::GetTextLineHeight() + framePaddingY });
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

        UI::Splitter(true, 1.0f, &m_FirstChildSize, &m_SecondChildSize, 10.0f, 80.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 7 });
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::WindowBg);
        ImGui::BeginChild("##FileStructurePanel", ImVec2(m_FirstChildSize, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleColor();

        for (auto& directory : std::filesystem::directory_iterator("Assets")) {
            if (directory.is_directory()) {
                RecursivelyAddDirectoryNodes(directory, std::filesystem::directory_iterator(directory));
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::SameLine();

        float topChildHeight = 38.0f;
        float bottomChildHeight = ImGui::GetContentRegionAvail().y - topChildHeight;

        ImGui::BeginChild("##ContentBrowserTopBar", ImVec2(m_SecondChildSize, topChildHeight), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        float arrowSize = 18.0f;
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FL_BACK_ARROW_ICON]->CreateOrGetDescriptorSet()), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Assets")
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FL_FORWARD_ARROW_ICON]->CreateOrGetDescriptorSet()), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Assets")
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();
        if (m_IsSearchBoxFocused)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
        UI::SearchBar("##ContentBrowserSearchBar", 150.0f, m_SearchInputBuffer, 256, "Search...");
        ImGui::PopStyleVar(2);

        if (m_IsSearchBoxFocused)
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        m_IsSearchBoxFocused = ImGui::IsItemActive() && ImGui::IsItemFocused();

        ImGui::SameLine();

        auto bigFont = ImGui::GetIO().Fonts->Fonts[0];
        ImGui::PushFont(bigFont);
        ImGui::Text("%s", m_CurrentDirectory.c_str());
        ImGui::PopFont();

        ImVec2 pos = ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowHeight() + ImGui::GetWindowPos().y);
        ImGui::GetWindowDrawList()->AddLine(pos, ImVec2{ pos.x + ImGui::GetWindowWidth(), pos.y }, 0xff101010, 4.0f);
        ImGui::EndChild();

        ImGui::SetNextWindowPos(pos);

        ImGui::BeginChild("##Contents", ImVec2(m_SecondChildSize, bottomChildHeight), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleVar();

        float iconWidth = 80.0f, padding = 12.0f;
        float cellSize = iconWidth + padding;
        uint32_t columns = ImGui::GetContentRegionAvail().x / cellSize;
        columns = columns >= 1 ? columns : 1;
        ImGui::Columns(columns, (const char*)__null, false);

        ImVec2 itemSize;
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
            bool isFileSupported = true, isDirectory = directory.is_directory();

            if (isDirectory)
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
                isFileSupported = false;
            }
            
            itemSize = UI::ContentBrowserItem(filePath, iconWidth, iconWidth, m_IconTextures[currentIconIndex]->CreateOrGetDescriptorSet());

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && isDirectory)
                m_CurrentDirectory = directory.path();

            if (ImGui::GetColumnIndex() == columns - 1)
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);

            ImGui::NextColumn();
            ImGui::PopID();
        }

        if (ImGui::GetColumnIndex() != 0)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + itemSize.y + 10.0f);

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
                platform::OpenInExplorerOrFinder((m_ProjectDirectory / m_CurrentDirectory).string().c_str());
            ImGui::EndPopup();
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
