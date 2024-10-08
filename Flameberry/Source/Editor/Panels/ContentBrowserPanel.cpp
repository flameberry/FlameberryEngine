#include "ContentBrowserPanel.h"

#include <filesystem>
#include <IconFontCppHeaders/IconsLucide.h>

#include "Flameberry.h"
#include "Core/UI.h"
#include "Project/Project.h"
#include "imgui.h"

#define FBY_BACK_ARROW_ICON 0
#define FBY_FORWARD_ARROW_ICON 1
#define FBY_SETTINGS_ICON 9

enum FileTypeIndex
{
	DEFAULT = 2,
	FOLDER,
	BERRY,
	OBJ,
	GLTF,
	FBX,
	FBMAT
};

static std::vector<std::string> g_IconPaths = {
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/ArrowBackIcon.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/ArrowNextIcon.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/FileIconDefault.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/FolderIconYellow.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/FileIconBerry.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/FileIconOBJ.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/FileIconGLTF.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/FileIconFBX.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/FileIconFBMAT.png",
	FBY_PROJECT_DIR "Flameberry/Assets/Icons/SettingsIcon.png"
};

namespace Flameberry {

	ContentBrowserPanel::ContentBrowserPanel()
		: m_CurrentDirectory(Project::GetActiveProject()->GetConfig().AssetDirectory) // Getting Asset Directory via this method to get the relative path only
		, m_VkTextureSampler(Texture2D::GetDefaultSampler())
	{
		for (const auto& path : g_IconPaths)
			m_IconTextures.emplace_back(CreateRef<Texture2D>(path.c_str(), m_VkTextureSampler));
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
	}

	void ContentBrowserPanel::RecursivelyAddDirectoryNodes(const std::filesystem::directory_entry& parent, const std::filesystem::directory_iterator& iterator)
	{
		constexpr float framePaddingY = 2.5f;

		bool isLeaf = true;

		for (auto& directory : std::filesystem::directory_iterator{ parent })
			isLeaf = isLeaf && !directory.is_directory();

		const bool isSelected = m_CurrentDirectory == parent;

		const int treeNodeFlags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0)
			| ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_FramePadding
			| (isLeaf ? ImGuiTreeNodeFlags_Leaf : 0);

		ImGui::PushID(parent.path().filename().c_str());

		if (IsPathInHierarchy(m_CurrentDirectory, parent))
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);

		bool isOpened;

		{
			UI::ScopedStyleVariable framePadding(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, framePaddingY });
			UI::ScopedStyleVariable itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 1, 0 });

			UI::ScopedStyleColor header(ImGuiCol_Header, Theme::AccentColor);
			UI::ScopedStyleColor headerActive(ImGuiCol_HeaderActive, Theme::AccentColorLight);
			UI::ScopedStyleColor headerHovered(ImGuiCol_HeaderHovered, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f }, isSelected);

			const ImVec4 textColor = isSelected ? ImVec4(0, 0, 0, 1) : ImGui::GetStyle().Colors[ImGuiCol_Text];
			UI::ScopedStyleColor _(ImGuiCol_Text, textColor);

			isOpened = ImGui::TreeNodeEx("##node", treeNodeFlags);
		}

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			m_CurrentDirectory = parent.path();

		ImGui::SameLine();
		ImGui::Image(reinterpret_cast<ImTextureID>(m_IconTextures[FileTypeIndex::FOLDER]->CreateOrGetDescriptorSet()), ImVec2{ ImGui::GetTextLineHeight() + framePaddingY, ImGui::GetTextLineHeight() + framePaddingY });
		ImGui::SameLine();

		{
			const ImVec4 textColor = isSelected ? ImVec4(0, 0, 0, 1) : ImGui::GetStyle().Colors[ImGuiCol_Text];
			UI::ScopedStyleColor _(ImGuiCol_Text, textColor);

			std::string filename = parent.path().filename().string();
			ImGui::Text("%s", filename.c_str());
		}

		ImGui::PopID();

		if (isOpened)
		{
			for (auto& directory : iterator)
			{
				if (directory.is_directory())
					RecursivelyAddDirectoryNodes(directory, std::filesystem::directory_iterator{ directory });
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

		for (int32_t i = (int32_t)(parentstr.size() - 1); i >= 0; i--)
		{
			if (keystr[i] != parentstr[i])
				return false;
		}
		return true;
	}

	void ContentBrowserPanel::OnUIRender()
	{
		{
			UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("Content Browser");
		}

		m_SecondChildSize = ImGui::GetContentRegionAvail().x - m_FirstChildSize - 8.0f;

		UI::Splitter(true, 1.2f, &m_FirstChildSize, &m_SecondChildSize, 10.0f, 80.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 7 });
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);

		ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::WindowBgGrey);
		ImGui::BeginChild("##FileStructurePanel", ImVec2(m_FirstChildSize, -1.0f), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_AlwaysUseWindowPadding);
		ImGui::PopStyleColor();

		for (auto& directory : std::filesystem::directory_iterator(Project::GetActiveProject()->GetConfig().AssetDirectory))
		{
			if (directory.is_directory())
			{
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

		constexpr float topChildHeight = 34.0f;
		const float bottomChildHeight = ImGui::GetContentRegionAvail().y - topChildHeight;

		ImGui::BeginChild("##ContentBrowserTopBar", ImVec2(m_SecondChildSize, topChildHeight), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleVar();

		constexpr float arrowSize = 14.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FBY_BACK_ARROW_ICON]->CreateOrGetDescriptorSet()), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Content")
			m_CurrentDirectory = m_CurrentDirectory.parent_path();
		ImGui::SameLine(0.0f, 0.0f);
		if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_IconTextures[FBY_FORWARD_ARROW_ICON]->CreateOrGetDescriptorSet()), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Content")
			m_CurrentDirectory = m_CurrentDirectory.parent_path();

		ImGui::SameLine();

		{
			UI::ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 1.0f, m_IsSearchBoxFocused);
			UI::ScopedStyleColor borderColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });

			UI::InputBox("##ContentBrowserSearchBar", 150.0f, &m_SearchInputBuffer, ICON_LC_SEARCH " Search...");
		}

		m_IsSearchBoxFocused = ImGui::IsItemActive() && ImGui::IsItemFocused();

		ImGui::SameLine();

		UI_CurrentPathBar();

		// Icon Size controller
		auto& style = ImGui::GetStyle();
		const float totalIconWidth = arrowSize + 2.0f * style.FramePadding.x + style.ItemSpacing.x;
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowSize().x - totalIconWidth);

		ImGui::ImageButton("##ContentBrowserPanelSettingsButton", reinterpret_cast<ImTextureID>(m_IconTextures[FBY_SETTINGS_ICON]->CreateOrGetDescriptorSet()), ImVec2(arrowSize, arrowSize), ImVec2(0, 0), ImVec2(1.0f, 1.0f));

		ImGui::PopStyleColor();

		if (ImGui::IsItemClicked())
			ImGui::OpenPopup("##ContentBrowserPanelSettingsPopup");

		if (ImGui::BeginPopup("##ContentBrowserPanelSettingsPopup"))
		{
			ImGui::PushItemWidth(50.0f);
			ImGui::DragInt("Thumbnail Size", (int32_t*)&m_ThumbnailSize, 1.0f, 50, 400);
			ImGui::PopItemWidth();
			ImGui::EndPopup();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 7 });

		// Separator
		ImVec2 pos = ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowHeight() + ImGui::GetWindowPos().y);
		ImGui::GetWindowDrawList()->AddLine(pos, ImVec2{ pos.x + ImGui::GetWindowWidth(), pos.y }, 0xff141414, 2.0f);

		ImGui::EndChild();

		ImGui::SetNextWindowPos(pos);
		ImGui::BeginChild("##Contents", ImVec2(m_SecondChildSize, bottomChildHeight), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_AlwaysUseWindowPadding);
		ImGui::PopStyleVar();

		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float cellSize = m_ThumbnailSize + spacing;
		uint32_t columns = ImGui::GetContentRegionAvail().x / cellSize, rowIndex = 0;
		columns = columns >= 1 ? columns : 1;
		ImGui::Columns(columns, (const char*)nullptr, false);

		ImVec2 itemSize;

		Project::GetActiveProject()->GetThumbnailCache()->ResetThumbnailLoadedCounter();

		for (const auto& directory : std::filesystem::directory_iterator{ m_CurrentDirectory })
		{
			const std::filesystem::path& filePath = directory.path();

			if (filePath.filename().string() == ".DS_Store")
				continue;

			if (m_SearchInputBuffer[0] != '\0')
			{
				// TODO: Maybe some optimisation to not search again if the input string is same
				const std::string filePathWithoutExtension = filePath.filename().replace_extension().string();
				const int index = Algorithm::KmpSearch(filePathWithoutExtension.c_str(), m_SearchInputBuffer.c_str(), true);
				if (index == -1)
					continue;
			}

			ImGui::PushID(filePath.filename().c_str());
			const std::string ext = filePath.extension().string();
			bool isFileSupported = true, isDirectory = directory.is_directory();

			Ref<Texture2D> thumbnail;
			if (!isDirectory)
				thumbnail = Project::GetActiveProject()->GetThumbnailCache()->GetOrCreateThumbnail(filePath);
			if (!thumbnail)
			{
				int currentIconIndex;
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
				else
				{
					currentIconIndex = FileTypeIndex::DEFAULT;
					isFileSupported = false;
				}
				thumbnail = m_IconTextures[currentIconIndex];
			}

			if (UI::ContentBrowserItem(filePath, m_ThumbnailSize, thumbnail, itemSize, !isFileSupported) && isDirectory)
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

		if (ImGui::BeginPopupContextWindow((const char*)nullptr, m_PopupFlags))
		{
			if (ImGui::BeginMenu(ICON_LC_PLUS "\tCreate"))
			{
				if (ImGui::MenuItem(ICON_LC_DRIBBBLE "\tMaterial"))
				{
					auto mat = CreateRef<MaterialAsset>("New Material");
					MaterialAssetSerializer::Serialize(mat, m_CurrentDirectory / "NewMaterial.fbmat");
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem(ICON_LC_EXTERNAL_LINK "\tOpen In Finder"))
			{
				const std::string filePathStr = (Project::GetActiveProjectDirectory() / m_CurrentDirectory).string();
				Platform::OpenInExplorerOrFinder(filePathStr.c_str());
			}
			ImGui::EndPopup();
		}
		ImGui::EndChild();
		ImGui::End();
	}

	void ContentBrowserPanel::UI_CurrentPathBar()
	{
		const auto& style = ImGui::GetStyle();
		const float totalIconWidth = 14.0f + 2.0f * style.FramePadding.x + 2.0f * style.ItemSpacing.x;
		const float currentPathItemWidth = ImGui::GetContentRegionAvail().x - totalIconWidth;

		const ImVec2 cursorPositionRelativeStart = ImGui::GetCursorScreenPos();
		const ImRect clipRect(cursorPositionRelativeStart, cursorPositionRelativeStart + ImVec2(currentPathItemWidth, ImGui::GetFontSize() + 2.0f * style.FramePadding.y));

		ImGui::PushClipRect(clipRect.Min, clipRect.Max, true);

		std::string currentDirectory = m_CurrentDirectory.string();
		std::string_view currentPath(currentDirectory.c_str());

		ImGui::GetWindowDrawList()->AddRectFilled(clipRect.Min, clipRect.Max,
			ImGui::ColorConvertFloat4ToU32(Theme::FrameBg),
			style.FrameRounding);
		ImGui::GetWindowDrawList()->AddRect(clipRect.Min, clipRect.Max,
			IM_COL32(70, 70, 70, 255),
			style.FrameRounding, 0, 0.5f);

		ImGui::SetCursorScreenPos(cursorPositionRelativeStart + ImVec2(2.0f * style.FramePadding.x, 0.0f));

		ImGui::AlignTextToFramePadding();

		std::size_t end = 0;

		// Initial path is "Content"
		// Later on path can be "Content/Textures", "Content/Textures/Texture1", "Content/Textures/Texture1/Texture2" etc.
		while (!currentPath.empty())
		{
			std::size_t position = currentPath.find_first_of(std::filesystem::path::preferred_separator);
			if (position == std::string::npos)
				position = currentPath.length();

			// Keeping the count of characters skipped throughout the entire string
			end += position;

			// Displaying the folder name
			const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
			ImGui::TextUnformatted(currentPath.data(), currentPath.data() + position);
			ImGui::SameLine();

			const ImRect buttonRect(cursorScreenPos - ImVec2(style.FramePadding.x, 0.0f),
				ImVec2(ImGui::GetCursorScreenPos().x - style.ItemSpacing.x + style.FramePadding.x,
					ImGui::GetCursorScreenPos().y + ImGui::GetFontSize() + 2.0f * style.FramePadding.y));

			const bool hovered = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(buttonRect.Min, buttonRect.Max, false);
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered)
			{
				m_CurrentDirectory = m_CurrentDirectory.string().substr(0, end);
				break;
			}

			if (hovered)
			{
				ImGui::GetWindowDrawList()->AddRect(
					buttonRect.Min, buttonRect.Max,
					ImGui::ColorConvertFloat4ToU32(Theme::AccentColor),
					style.FrameRounding);
			}

			currentPath.remove_prefix(position);

			if (!currentPath.empty())
			{
				end++;
				currentPath.remove_prefix(1);
				ImGui::Button(ICON_LC_CHEVRON_RIGHT);
				ImGui::SameLine();
			}
		}

		ImGui::PopClipRect();
	}

} // namespace Flameberry
