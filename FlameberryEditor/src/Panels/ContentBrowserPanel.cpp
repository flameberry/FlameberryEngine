#include "ContentBrowserPanel.h"

#include "Flameberry.h"
#include "UI.h"

#define FBY_BACK_ARROW_ICON 0
#define FBY_FORWARD_ARROW_ICON 1

enum FileTypeIndex {
    DEFAULT = 2,
    FOLDER, BERRY, OBJ, GLTF, FBX, FBMAT
};

static std::vector<std::string> g_IconPaths = {
    FBY_PROJECT_DIR"FlameberryEditor/icons/arrow_back.png",
    FBY_PROJECT_DIR"FlameberryEditor/icons/arrow_forward.png",
    FBY_PROJECT_DIR"FlameberryEditor/icons/FileIconDefault.png",
    FBY_PROJECT_DIR"FlameberryEditor/icons/folder_icon.png",
    FBY_PROJECT_DIR"FlameberryEditor/icons/FileIconBerry.png",
    FBY_PROJECT_DIR"FlameberryEditor/icons/FileIconOBJ.png",
    FBY_PROJECT_DIR"FlameberryEditor/icons/FileIconGLTF.png",
    FBY_PROJECT_DIR"FlameberryEditor/icons/FileIconFBX.png",
    FBY_PROJECT_DIR"FlameberryEditor/icons/FileIconFBMAT.png"
};

namespace Flameberry {
    ContentBrowserPanel::ContentBrowserPanel()
        : m_CurrentDirectory(Project::GetActiveProject()->GetConfig().AssetDirectory), // Getting Asset Directory via this method to get the relative path only
        m_ThumbnailCache(std::make_shared<ThumbnailCache>(Project::GetActiveProject())),
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

        for (int32_t i = int32_t(parentstr.size() - 1); i >= 0; i--)
        {
            if (keystr[i] != parentstr[i])
                return false;
        }
        return true;
    }

    void ContentBrowserPanel::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Content Browser");
        ImGui::PopStyleVar();

        m_SecondChildSize = ImGui::GetContentRegionAvail().x - m_FirstChildSize - 8.0f;

        UI::Splitter(true, 1.2f, &m_FirstChildSize, &m_SecondChildSize, 10.0f, 80.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 7 });
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::WindowBgGrey);
        ImGui::BeginChild("##FileStructurePanel", ImVec2(m_FirstChildSize, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleColor();

        for (auto& directory : std::filesystem::directory_iterator("Assets")) {
            if (directory.is_directory()) {
                RecursivelyAddDirectoryNodes(directory, std::filesystem::directory_iterator(directory));
            }
        }

        // Add Shadow Effect
        constexpr float shadowWidth = 18.0f;
        const float shadowXMax = ImGui::GetWindowPos().x + ImGui::GetWindowWidth();
        const float shadowYMax = ImGui::GetWindowPos().y;
        ImVec2 pMin(shadowXMax - shadowWidth, shadowYMax + ImGui::GetWindowHeight());
        ImVec2 pMax(shadowXMax, shadowYMax);
        
        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(pMin, pMax, IM_COL32(5, 5, 5, 0), IM_COL32(5, 5, 5, 140), IM_COL32(5, 5, 5, 140), IM_COL32(5, 5, 5, 0));
        
        ImGui::EndChild();
        ImGui::PopStyleVar();
        
        ImGui::SameLine();

        float topChildHeight = 38.0f;
        float bottomChildHeight = ImGui::GetContentRegionAvail().y - topChildHeight;

        ImGui::BeginChild("##ContentBrowserTopBar", ImVec2(m_SecondChildSize, topChildHeight), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        float arrowSize = 18.0f;
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FBY_BACK_ARROW_ICON]->CreateOrGetDescriptorSet()), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Assets")
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FBY_FORWARD_ARROW_ICON]->CreateOrGetDescriptorSet()), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Assets")
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        ImGui::SameLine();
        if (m_IsSearchBoxFocused)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
        UI::InputBox("##ContentBrowserSearchBar", 150.0f, m_SearchInputBuffer, 256, "Search...");
        ImGui::PopStyleVar(2);

        if (m_IsSearchBoxFocused)
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        m_IsSearchBoxFocused = ImGui::IsItemActive() && ImGui::IsItemFocused();

        ImGui::SameLine();

        ImGui::Text("%s", m_CurrentDirectory.c_str());

        ImVec2 pos = ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowHeight() + ImGui::GetWindowPos().y);
        ImGui::GetWindowDrawList()->AddLine(pos, ImVec2{ pos.x + ImGui::GetWindowWidth(), pos.y }, 0xff101010, 4.0f);
        ImGui::EndChild();

        ImGui::SetNextWindowPos(pos);

        ImGui::BeginChild("##Contents", ImVec2(m_SecondChildSize, bottomChildHeight), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleVar();

        float iconWidth = 90.0f, padding = 0.0f;
        float cellSize = iconWidth + padding;
        uint32_t columns = ImGui::GetContentRegionAvail().x / cellSize, rowIndex = 0;
        columns = columns >= 1 ? columns : 1;
        ImGui::Columns(columns, (const char*)__null, false);

        ImVec2 itemSize;
        
        m_ThumbnailCache->ResetThumbnailLoadedCounter();
        
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
            
            std::shared_ptr<Texture2D> thumbnail;
            if (!isDirectory)
                thumbnail = m_ThumbnailCache->TryGetOrCreateThumbnail(filePath);
            if (!thumbnail)
            {
                if (isDirectory)
                    currentIconIndex = FileTypeIndex::FOLDER;
                else if (ext == ".berry")
                    currentIconIndex = FileTypeIndex::BERRY;
                else if (ext == ".obj")
                    currentIconIndex = FileTypeIndex::OBJ;
                else if (ext == ".gltf")
                    currentIconIndex = FileTypeIndex::GLTF;
                else if (ext == ".fbx")
                    currentIconIndex = FileTypeIndex::FBX;
                else if (ext == ".fbmat")
                    currentIconIndex = FileTypeIndex::FBMAT;
                else {
                    currentIconIndex = FileTypeIndex::DEFAULT;
                    isFileSupported = false;
                }
                thumbnail = m_IconTextures[currentIconIndex];
            }

            itemSize = UI::ContentBrowserItem(filePath, iconWidth, thumbnail, !isFileSupported);

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && isDirectory)
                m_CurrentDirectory = directory.path();

            if (ImGui::GetColumnIndex() == columns - 1)
            {
                rowIndex++;
                constexpr float paddingY = 20.0f;
                ImGui::SetCursorPosY(rowIndex * (itemSize.y + paddingY));
            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        if (ImGui::GetColumnIndex() != 0)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + itemSize.y + 10.0f);

        if (ImGui::BeginPopupContextWindow((const char*)__null, m_PopupFlags))
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
                platform::OpenInExplorerOrFinder((Project::GetActiveProjectDirectory() / m_CurrentDirectory).string().c_str());
            ImGui::EndPopup();
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
