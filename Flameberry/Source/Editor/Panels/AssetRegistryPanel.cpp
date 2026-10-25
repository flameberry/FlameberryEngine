#include "AssetRegistryPanel.h"

#include <algorithm>
#include <filesystem>
#include <vector>

#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <IconFontCppHeaders/IconsLucide.h>
#include <fmt/format.h>

#include "Asset/AssetManager.h"
#include "Asset/EditorAssetManager.h"
#include "Core/Algorithm.h"
#include "Core/UI.h"
#include "Project/Project.h"
#include "ImGui/Theme.h"

namespace Flameberry {

	namespace {

		enum AssetRegistryColumn : ImGuiID {
			AssetRegistryColumn_Type = 0,
			AssetRegistryColumn_Name,
			AssetRegistryColumn_Path,
			AssetRegistryColumn_Handle,
		};

		struct AssetTypeStyle {
			const char* Name;
			const char* Icon;
			ImVec4 Color;
		};

		// Each asset type gets its own icon color, so that the types can be told apart at a glance
		AssetTypeStyle GetAssetTypeStyle(AssetType type) {
			switch (type) {
				case AssetType::Scene:
					return { "Scene", ICON_LC_CLAPPERBOARD, ThemeUtils::RGB(236, 150, 76) };
				case AssetType::Texture2D:
					return { "Texture", ICON_LC_IMAGE, ThemeUtils::RGB(126, 190, 102) };
				case AssetType::StaticMesh:
					return { "Static Mesh", ICON_LC_BOX, ThemeUtils::RGB(98, 160, 230) };
				case AssetType::Material:
					return { "Material", ICON_LC_PALETTE, ThemeUtils::RGB(206, 122, 200) };
				case AssetType::Skymap:
					return { "Skymap", ICON_LC_SUNRISE, ThemeUtils::RGB(92, 196, 200) };
				case AssetType::Font:
					return { "Font", ICON_LC_TYPE, ThemeUtils::RGB(200, 200, 200) };
				case AssetType::None:
					break;
			}
			return { "Unknown", ICON_LC_FILE, Theme::TextMuted };
		}

		constexpr AssetType s_FilterableAssetTypes[] = { AssetType::Scene, AssetType::Texture2D, AssetType::StaticMesh,
			AssetType::Material, AssetType::Skymap, AssetType::Font };

		struct AssetRow {
			AssetHandle Handle;
			const AssetMetadata* Metadata;
			std::string Name, DisplayPath;
		};

		// Displays the text vertically centered in a frame-height row, cut off with an ellipsis if it doesn't fit the
		// available width, in which case the full text is shown as a tooltip
		void TextEllipsized(const char* text) {
			const ImVec2 pos = ImGui::GetCursorScreenPos() + ImVec2(0.0f, ImGui::GetStyle().FramePadding.y);
			const float maxX = pos.x + ImGui::GetContentRegionAvail().x;
			const ImVec2 textSize = ImGui::CalcTextSize(text);

			ImGui::RenderTextEllipsis(
				ImGui::GetWindowDrawList(), pos, ImVec2(maxX, pos.y + textSize.y), maxX, text, nullptr, &textSize);
			ImGui::Dummy(ImVec2(ImMin(textSize.x, maxX - pos.x), ImGui::GetFrameHeight()));

			if (textSize.x > maxX - pos.x && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
				ImGui::SetTooltip("%s", text);
		}

	} // namespace

	void AssetRegistryPanel::OnUIRender() {
		ImGui::Begin("Asset Registry");

		const auto& registry = AssetManager::As<EditorAssetManager>()->GetAssetRegistry();

		// Gather the assets that pass the type filter and the search
		// Paths inside the project are displayed relative to the project directory, which is far shorter
		const std::filesystem::path& projectDirectory = Project::GetActiveProject()->GetProjectDirectory();

		std::vector<AssetRow> rows;
		rows.reserve(registry.size());
		for (const auto& [handle, metadata] : registry) {
			if (m_TypeFilter != AssetType::None && metadata.Type != m_TypeFilter) continue;

			// The file path is used as the name of memory only assets
			std::string name =
				metadata.IsMemoryAsset ? metadata.FilePath : std::filesystem::path(metadata.FilePath).filename().string();

			if (!m_SearchInputBuffer.empty()) {
				const std::string handleStr = fmt::format("{}", (UUID::ValueType)handle);
				const char* search = m_SearchInputBuffer.c_str();
				const bool matches = Algorithm::KmpSearch(name.c_str(), search, true) != -1
					|| Algorithm::KmpSearch(metadata.FilePath.c_str(), search, true) != -1
					|| Algorithm::KmpSearch(handleStr.c_str(), search, true) != -1;
				if (!matches) continue;
			}

			std::string displayPath = metadata.FilePath;
			if (!metadata.IsMemoryAsset) {
				const std::filesystem::path relativePath =
					std::filesystem::path(metadata.FilePath).lexically_relative(projectDirectory);
				if (!relativePath.empty() && *relativePath.begin() != "..") displayPath = relativePath.string();
			}

			rows.push_back({ handle, &metadata, std::move(name), std::move(displayPath) });
		}

		DisplayToolbar(rows.size(), registry.size());

		constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable
			| ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX;

		if (ImGui::BeginTable("##AssetRegistryTable", 4, tableFlags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 110.0f, AssetRegistryColumn_Type);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 1.2f,
				AssetRegistryColumn_Name);
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch, 1.0f, AssetRegistryColumn_Path);
			// Handles are rarely needed when browsing, so the column starts hidden (right-click the header to show it)
			ImGui::TableSetupColumn("Handle", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 170.0f,
				AssetRegistryColumn_Handle);
			ImGui::TableHeadersRow();

			// The rows are gathered every frame, so they are sorted every frame too
			if (const ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
				std::sort(rows.begin(), rows.end(), [sortSpecs](const AssetRow& a, const AssetRow& b) {
					for (int i = 0; i < sortSpecs->SpecsCount; i++) {
						const ImGuiTableColumnSortSpecs& spec = sortSpecs->Specs[i];
						int delta = 0;
						switch (spec.ColumnUserID) {
							case AssetRegistryColumn_Type:
								delta = (int)a.Metadata->Type - (int)b.Metadata->Type;
								break;
							case AssetRegistryColumn_Name:
								delta = a.Name.compare(b.Name);
								break;
							case AssetRegistryColumn_Path:
								delta = a.DisplayPath.compare(b.DisplayPath);
								break;
							case AssetRegistryColumn_Handle:
								delta = (UUID::ValueType)a.Handle < (UUID::ValueType)b.Handle
									? -1
									: ((UUID::ValueType)b.Handle < (UUID::ValueType)a.Handle ? 1 : 0);
								break;
						}
						if (delta != 0) return spec.SortDirection == ImGuiSortDirection_Ascending ? delta < 0 : delta > 0;
					}
					return false;
				});
			}

			const ImGuiStyle& style = ImGui::GetStyle();
			const float rowHeight = ImGui::GetFrameHeight() + 2.0f * style.CellPadding.y;

			ImGuiListClipper clipper;
			clipper.Begin(rows.size(), rowHeight);
			while (clipper.Step()) {
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					const AssetRow& row = rows[i];
					const UUID::ValueType handleValue = row.Handle;
					const bool isSelected = (UUID::ValueType)m_SelectedHandle == handleValue;
					const AssetTypeStyle typeStyle = GetAssetTypeStyle(row.Metadata->Type);

					ImGui::PushID((const void*)(uintptr_t)handleValue);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					UI::ScopedStyleColor rowText(ImGuiCol_Text, Theme::SelectionTextColor, isSelected);

					// The row's selectable spans all the columns, and the cell contents are drawn over it
					const ImVec2 cellPos = ImGui::GetCursorScreenPos();
					{
						UI::ScopedStyleColor header(ImGuiCol_Header, Theme::SelectionColor, isSelected);
						UI::ScopedStyleColor headerHovered(ImGuiCol_HeaderHovered, Theme::SelectionColorHovered, isSelected);
						UI::ScopedStyleColor headerActive(ImGuiCol_HeaderActive, Theme::SelectionColorActive, isSelected);

						constexpr ImGuiSelectableFlags selectableFlags =
							ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
						if (ImGui::Selectable(
								"##AssetRow", isSelected, selectableFlags, ImVec2(0.0f, ImGui::GetFrameHeight())))
							m_SelectedHandle = row.Handle;
					}

					if (ImGui::BeginPopupContextItem("##AssetRowContextMenu")) {
						m_SelectedHandle = row.Handle;

						UI::ScopedStyleColor menuText(ImGuiCol_Text, Theme::TextColor);
						if (ImGui::MenuItem(ICON_LC_COPY "  Copy Handle"))
							ImGui::SetClipboardText(fmt::format("{}", handleValue).c_str());
						if (!row.Metadata->IsMemoryAsset && ImGui::MenuItem(ICON_LC_COPY "  Copy Path"))
							ImGui::SetClipboardText(row.Metadata->FilePath.c_str());
						ImGui::EndPopup();
					}

					// Type
					ImGui::SetCursorScreenPos(cellPos);
					ImGui::AlignTextToFramePadding();
					ImGui::TextColored(isSelected ? Theme::SelectionTextColor : typeStyle.Color, "%s", typeStyle.Icon);
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted(typeStyle.Name);

					// Name
					ImGui::TableNextColumn();
					TextEllipsized(row.Name.c_str());

					// Path
					ImGui::TableNextColumn();
					{
						UI::ScopedStyleColor pathText(ImGuiCol_Text, Theme::TextMuted, !isSelected);
						if (row.Metadata->IsMemoryAsset) {
							ImGui::AlignTextToFramePadding();
							ImGui::TextUnformatted("In memory");
						} else
							TextEllipsized(row.DisplayPath.c_str());
					}

					// Handle
					ImGui::TableNextColumn();
					{
						UI::ScopedStyleColor handleText(ImGuiCol_Text, Theme::TextMuted, !isSelected);
						ImGui::AlignTextToFramePadding();
						ImGui::Text("%llu", handleValue);
					}

					ImGui::PopID();
				}
			}

			if (rows.empty()) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TextDisabled(registry.empty() ? "The asset registry is empty" : "No assets match the filters");
			}

			ImGui::EndTable();
		}

		ImGui::End();
	}

	void AssetRegistryPanel::DisplayToolbar(uint32_t visibleAssetCount, uint32_t totalAssetCount) {
		const float searchBoxWidth = std::min(240.0f, 0.4f * ImGui::GetContentRegionAvail().x);
		m_IsSearchBoxFocused = UI::InputBox("##AssetRegistrySearchBar", searchBoxWidth, &m_SearchInputBuffer,
			ICON_LC_SEARCH " Search...", m_IsSearchBoxFocused);

		ImGui::SameLine();

		// Type filter
		{
			const AssetTypeStyle typeStyle = GetAssetTypeStyle(m_TypeFilter);
			const std::string label = m_TypeFilter == AssetType::None
				? std::string(ICON_LC_LAYERS "  All Types  " ICON_LC_CHEVRON_DOWN)
				: fmt::format("{}  {}  " ICON_LC_CHEVRON_DOWN, typeStyle.Icon, typeStyle.Name);
			if (ImGui::Button(label.c_str())) UI::OpenSelectionWidget("##AssetTypeFilter");

			if (UI::BeginSelectionWidget("##AssetTypeFilter", "Filter by Type", &m_TypeFilterSearchBuffer)) {
				if (UI::SelectionWidgetElement("All Types", m_TypeFilter == AssetType::None, ICON_LC_LAYERS))
					m_TypeFilter = AssetType::None;

				for (const AssetType type : s_FilterableAssetTypes) {
					const AssetTypeStyle style = GetAssetTypeStyle(type);
					if (UI::SelectionWidgetElement(style.Name, m_TypeFilter == type, style.Icon)) m_TypeFilter = type;
				}
				UI::EndSelectionWidget();
			}
		}

		// Asset count, aligned to the right end of the toolbar unless the panel is too narrow to fit it
		{
			const std::string count = visibleAssetCount == totalAssetCount
				? fmt::format("{} assets", totalAssetCount)
				: fmt::format("{} of {} assets", visibleAssetCount, totalAssetCount);
			const float countWidth = ImGui::CalcTextSize(count.c_str()).x;

			ImGui::SameLine();
			const float countX = ImGui::GetWindowContentRegionMax().x - countWidth;
			if (countX > ImGui::GetCursorPosX()) {
				ImGui::SetCursorPosX(countX);
				ImGui::AlignTextToFramePadding();
				ImGui::TextColored(Theme::TextMuted, "%s", count.c_str());
			} else
				ImGui::NewLine();
		}
	}

} // namespace Flameberry
