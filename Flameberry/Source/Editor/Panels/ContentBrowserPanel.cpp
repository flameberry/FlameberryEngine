#include "ContentBrowserPanel.h"

#include <filesystem>
#include <IconFontCppHeaders/IconsLucide.h>

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

	namespace Utils {

		std::string FormatFileSize(uintmax_t sizeBytes)
		{
			const char* sizes[] = { "B", "KB", "MB", "GB", "TB" };
			int order = 0;
			double size = static_cast<double>(sizeBytes);

			while (size >= 1024.0 && order < 4)
			{
				order++;
				size /= 1024.0;
			}

			return fmt::format("{:.2f} {}", size, sizes[order]);
		}

	} // namespace Utils

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

		ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::WindowBg);
		ImGui::BeginChild("##FileStructurePanel", ImVec2(m_FirstChildSize, -1.0f), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_AlwaysUseWindowPadding);
		ImGui::PopStyleColor();

		for (auto& directory : std::filesystem::directory_iterator(Project::GetActiveProject()->GetConfig().AssetDirectory))
		{
			if (directory.is_directory())
				RecursivelyAddDirectoryNodes(directory, std::filesystem::directory_iterator(directory));
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

		if (ImGui::ImageButton("##BackArrow", (ImTextureID)m_IconTextures[FBY_BACK_ARROW_ICON]->CreateOrGetDescriptorSet(), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Content")
			m_CurrentDirectory = m_CurrentDirectory.parent_path();
		ImGui::SameLine(0.0f, 0.0f);
		if (ImGui::ImageButton("##ForwardArrow", (ImTextureID)m_IconTextures[FBY_FORWARD_ARROW_ICON]->CreateOrGetDescriptorSet(), ImVec2{ arrowSize, arrowSize }) && m_CurrentDirectory != "Content")
			m_CurrentDirectory = m_CurrentDirectory.parent_path();

		ImGui::SameLine();

		{
			UI::InputBox("##ContentBrowserSearchBar", 150.0f, &m_SearchInputBuffer, ICON_LC_SEARCH " Search...", m_IsSearchBoxFocused);
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

			if (DisplayContentBrowserItem(filePath, m_ThumbnailSize, thumbnail, itemSize, !isFileSupported) && isDirectory)
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

	bool ContentBrowserPanel::DisplayContentBrowserItem(const std::filesystem::path& filepath, float size, const Ref<Texture2D>& thumbnail, ImVec2& outItemSize, bool keepExtension)
	{
		std::string filePathStr = filepath.string();
		const char* filePathCStrID = filePathStr.c_str();
		const bool isDirectory = std::filesystem::is_directory(filepath);
		const auto& specification = thumbnail->GetImageSpecification();
		const float aspectRatio = (float)specification.Width / (float)specification.Height;

		const float width = size;
		float height = size;

		constexpr float borderThickness = 1.5f;
		const float thumbnailWidth = specification.Width >= specification.Height ? size - 2.0f * borderThickness : height * aspectRatio;
		const float thumbnailHeight = specification.Width >= specification.Height ? width / aspectRatio : size - 2.0f * borderThickness;

		ImGuiStyle& style = ImGui::GetStyle();
		const auto& framePadding = style.FramePadding;
		height += framePadding.y;

		const float textHeight = ImGui::GetTextLineHeightWithSpacing();
		const float fullWidth = width;
		const float fullHeight = height + 2 * textHeight + 2 * ImGui::GetStyle().ItemSpacing.y;
		const auto& cursorPos = ImGui::GetCursorScreenPos();

		bool hovered, held;
		ImRect bb = ImRect(cursorPos, cursorPos + ImVec2(fullWidth, fullHeight));
		ImGuiID id = ImGui::GetID(filePathCStrID);
		bool isDoubleClicked = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnDoubleClick);
		ImGui::ItemAdd(bb, id);

		if (!isDirectory)
		{
			ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, cursorPos + ImVec2(fullWidth, height), 0xff151515, 3, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(cursorPos.x, cursorPos.y + height), cursorPos + ImVec2(fullWidth, fullHeight), 0xff353535, 3, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersBottomRight);
			ImGui::GetWindowDrawList()->AddRect(cursorPos, cursorPos + ImVec2(fullWidth, fullHeight), hovered ? ImGui::ColorConvertFloat4ToU32(Theme::AccentColor) : 0xff000000, 3, 0, borderThickness);
		}
		else if (hovered)
		{
			constexpr float shadowThickness = 2.0f;
			constexpr ImVec2 offset(shadowThickness, shadowThickness);
			ImGui::GetWindowDrawList()->AddRect(cursorPos + offset, cursorPos + ImVec2(fullWidth, fullHeight) + offset, IM_COL32(25, 25, 25, 255), 3, 0, shadowThickness);
			ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, cursorPos + ImVec2(fullWidth, fullHeight), IM_COL32(60, 60, 60, 255), 3);
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload("FBY_CONTENT_BROWSER_ITEM", filePathStr.c_str(), (strlen(filePathStr.c_str()) + 1) * sizeof(char), ImGuiCond_Once);

			constexpr float size = 80.0f;

			// Show Asset Preview
			ImGui::Image((ImTextureID)thumbnail->CreateOrGetDescriptorSet(), ImVec2(size * aspectRatio, size));
			ImGui::SameLine();
			ImGui::Text("%s", filepath.stem().string().c_str());

			ImGui::EndDragDropSource();
		}
		else if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		{
			const std::string assetTypeStr = Utils::AssetTypeEnumToString(Utils::GetAssetTypeFromFileExtension(filepath.extension()));
			const std::string fileSizeStr = isDirectory ? "N/A" : Utils::FormatFileSize(std::filesystem::file_size(filepath));

			ImGui::BeginTooltip();
			ImGui::Text("Path: %s", filePathStr.c_str());
			ImGui::Text("Type: %s", isDirectory ? "Directory" : assetTypeStr.c_str());
			ImGui::Text("Size: %s", fileSizeStr.c_str());
			ImGui::EndTooltip();
		}

		if (ImGui::BeginPopupContextItem(filePathCStrID))
		{
			if (ImGui::MenuItem(ICON_LC_DELETE "\tDelete"))
			{
				// Add a confirm pop up
				// std::filesystem::remove(filepath);
				FBY_LOG("Delete");
			}
			ImGui::EndMenu();
		}

		ImGui::BeginGroup();

		const float centerTranslationWidth = width / 2.0f - thumbnailWidth / 2.0f;
		const float centerTranslationHeight = height / 2.0f - thumbnailHeight / 2.0f;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerTranslationWidth - framePadding.x);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + centerTranslationHeight - framePadding.y);

		{
			UI::ScopedStyleColor button(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			UI::ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
			UI::ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

			ImGui::ImageButton(filePathCStrID, (ImTextureID)thumbnail->CreateOrGetDescriptorSet(), ImVec2(thumbnailWidth, thumbnailHeight));
		}

		const auto& filename = keepExtension ? filepath.filename().string() : filepath.stem().string();
		const auto cursorPosX = ImGui::GetCursorPosX();
		ImGui::SetCursorPosX(cursorPosX + framePadding.x);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.ItemSpacing.y + centerTranslationHeight);

		const auto textWidth = ImGui::CalcTextSize(filename.c_str()).x;
		const auto aWidth = ImGui::CalcTextSize("a").x;
		const uint32_t characters = fullWidth / aWidth;

		// Format and align text based on whether the item is a directory or a file
		if (isDirectory)
		{
			if (textWidth > fullWidth)
				ImGui::Text("%.*s%s", characters, filename.c_str(), "...");
			else
			{
				ImGui::SetCursorPosX(glm::max(cursorPosX + framePadding.x, cursorPosX + (fullWidth - textWidth) * 0.5f));
				ImGui::Text("%s", filename.c_str());
			}
		}
		else
		{
			if (textWidth > 2.0f * fullWidth)
				ImGui::TextWrapped("%.*s%s", 2 * characters, filename.c_str(), "...");
			else
				ImGui::TextWrapped("%s", filename.c_str());
		}

		ImGui::EndGroup();

		outItemSize = ImVec2(fullWidth, fullHeight);
		return isDoubleClicked;
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
			ImGui::ColorConvertFloat4ToU32(Theme::ImGuiTitleBg),
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
