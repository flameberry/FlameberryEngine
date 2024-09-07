#include "Core/UI.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <IconFontCppHeaders/IconsLucide.h>

#include "Core/Core.h"
#include "Core/Algorithm.h"

#include "ImGui/Theme.h"

namespace Flameberry::UI {

	struct UIState
	{
		// Fixed Layout/Style Properties
		static constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_BordersInnerV
			| ImGuiTableFlags_BordersInnerH
			| ImGuiTableFlags_NoKeepColumnsVisible
			| ImGuiTableFlags_PadOuterX;

		static constexpr float LabelWidth = 100.0f;

		// Selection Widget
		bool IsSelectionWidgetJustOpened = false, IsSelectionWidgetSearchBoxFocused = false, HasSelectionWidgetListBoxBegun = false;
	};

	static UIState g_UIState;

	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size)
	{
		using namespace ImGui;
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiID id = window->GetID("##Splitter");
		ImRect bb;
		bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
		bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
		return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f, 0.0f, 0xFF000000);
	}

	bool AlignedButton(const char* label, const ImVec2& size, float alignment)
	{
		const ImGuiStyle& style = ImGui::GetStyle();

		const float width = size.x ? size.x : ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
		const float avail = ImGui::GetContentRegionAvail().x;

		const float off = (avail - width) * alignment;
		if (off > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		return ImGui::Button(label, size);
	}

	void InputBox(const char* label, const float width, std::string* inputBuffer, const char* inputHint)
	{
		ScopedStyleColor borderColor(ImGuiCol_Border, IM_COL32(70, 70, 70, 255));
		ScopedStyleVariable frameRounding(ImGuiStyleVar_FrameRounding, 4);
		ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 0.5f);

		ImGui::PushItemWidth(width);
		ImGui::InputTextWithHint(label, inputHint, inputBuffer);
		ImGui::PopItemWidth();
	}

	void OpenSelectionWidget(const char* label)
	{
		const std::string labelFmt = fmt::format("{}Popup", label);
		ImGui::OpenPopup(labelFmt.c_str());

		g_UIState.IsSelectionWidgetJustOpened = true;
	}

	bool BeginSelectionWidget(const char* label, std::string* inputBuffer)
	{
		ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));

		const std::string labelFmt = fmt::format("{}Popup", label);

		if (ImGui::BeginPopup(labelFmt.c_str()))
		{
			if (g_UIState.IsSelectionWidgetJustOpened)
			{
				ImGui::SetKeyboardFocusHere();
				g_UIState.IsSelectionWidgetJustOpened = false;
			}

			{
				ScopedStyleColor borderColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f }, g_UIState.IsSelectionWidgetSearchBoxFocused);
				ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 1.0f, g_UIState.IsSelectionWidgetSearchBoxFocused);

				const std::string inputBoxLabel = fmt::format("{}SearchBar", label);

				InputBox(inputBoxLabel.c_str(), -1.0f, inputBuffer, ICON_LC_SEARCH " Search...");
			}

			g_UIState.IsSelectionWidgetSearchBoxFocused = ImGui::IsItemActive() && ImGui::IsItemFocused();
			g_UIState.HasSelectionWidgetListBoxBegun = ImGui::BeginListBox("##ScriptActorClassesListBox");

			return true;
		}
		return false;
	}

	bool SelectionWidgetElement(const char* label, bool isSelected)
	{
		if (ImGui::Selectable(label, &isSelected))
		{
			ImGui::CloseCurrentPopup();
			return true;
		}

		if (isSelected)
			ImGui::SetItemDefaultFocus();

		return false;
	}

	void EndSelectionWidget()
	{
		if (g_UIState.HasSelectionWidgetListBoxBegun)
			ImGui::EndListBox();

		ImGui::EndPopup();
	}

	bool BeginKeyValueTable(const char* label, ImGuiTableFlags tableFlags, float labelWidth)
	{
		if (ImGui::BeginTable(label, 2, tableFlags ? tableFlags : g_UIState.TableFlags))
		{
			ImGui::TableSetupColumn("Attribute_Key", ImGuiTableColumnFlags_WidthFixed, labelWidth != 0.0f ? labelWidth : g_UIState.LabelWidth);
			ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);
			return true;
		}
		return false;
	}

	void TableKeyElement(const char* label)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label);
		ImGui::TableNextColumn();
	}

	void EndKeyValueTable()
	{
		ImGui::EndTable();
	}

	// TODO: Call one of 2 functions one for Folder Thumbnail and other for file
	bool ContentBrowserItem(const std::filesystem::path& filepath, float size, const Ref<Texture2D>& thumbnail, ImVec2& outItemSize, bool keepExtension)
	{
		std::string filePathStr = filepath.string();
		const char* filePathCStr = filePathStr.c_str();

		bool isDirectory = std::filesystem::is_directory(filepath);

		ImGuiStyle& style = ImGui::GetStyle();

		const auto& specification = thumbnail->GetImageSpecification();
		const float aspectRatio = (float)specification.Width / (float)specification.Height;

		const float width = size;
		float height = size;

		constexpr float borderThickness = 1.5f;

		const float thumbnailWidth = specification.Width >= specification.Height ? size - 2.0f * borderThickness : height * aspectRatio;
		const float thumbnailHeight = specification.Width >= specification.Height ? width / aspectRatio : size - 2.0f * borderThickness;

		const auto& framePadding = style.FramePadding;
		height += framePadding.y;

		const float textHeight = ImGui::GetTextLineHeightWithSpacing();
		const float fullWidth = width;
		const float fullHeight = height + 2 * textHeight;

		const auto& cursorPos = ImGui::GetCursorScreenPos();
		bool hovered, held;
		bool isDoubleClicked = ImGui::ButtonBehavior(ImRect(cursorPos, cursorPos + ImVec2(fullWidth, fullHeight)), ImGui::GetID(filepath.c_str()), &hovered, &held, ImGuiButtonFlags_PressedOnDoubleClick);

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

		ImGui::BeginGroup();

		const float centerTranslationWidth = width / 2.0f - thumbnailWidth / 2.0f;
		const float centerTranslationHeight = height / 2.0f - thumbnailHeight / 2.0f;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerTranslationWidth - framePadding.x);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + centerTranslationHeight - framePadding.y);

		{
			ScopedStyleColor button(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

			ImGui::ImageButton(filePathCStr, reinterpret_cast<ImTextureID>(thumbnail->CreateOrGetDescriptorSet()), ImVec2(thumbnailWidth, thumbnailHeight));
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

		if (ImGui::BeginPopupContextItem(filePathCStr))
		{
			if (ImGui::MenuItem(ICON_LC_DELETE "\tDelete"))
			{
				// Add a confirm pop up
				// std::filesystem::remove(filepath);
				FBY_LOG("Delete");
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload("FBY_CONTENT_BROWSER_ITEM", filePathCStr, (strlen(filePathCStr) + 1) * sizeof(char), ImGuiCond_Once);

			constexpr float size = 80.0f;

			// Show Asset Preview
			ImGui::Image(thumbnail->CreateOrGetDescriptorSet(), ImVec2(size * aspectRatio, size));
			ImGui::SameLine();
			ImGui::Text("%s", filepath.stem().string().c_str());

			ImGui::EndDragDropSource();
		}
		outItemSize = ImVec2(fullWidth, fullHeight);
		return isDoubleClicked;
	}

	bool ProjectRegistryEntryItem(const char* name, const char* path, bool disabled)
	{
		constexpr float paddingX = 15.0f, paddingY = 5.0f, spacing = 10.0f;
		const float itemWidth = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemWidth(itemWidth);

		if (disabled)
			ImGui::BeginDisabled();

		const auto& cursorScreenPos = ImGui::GetCursorScreenPos();

		ImGui::BeginGroup();
		ImVec2 cursorPos = ImGui::GetCursorPos();
		ImGui::SetCursorPosX(cursorPos.x + paddingX);
		ImGui::SetCursorPosY(cursorPos.y + 2.0f * paddingY);

		auto& bigFont = ImGui::GetIO().Fonts->Fonts[0];
		ImGui::Text("%s", name);

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX);
		ImGui::TextWrapped("%s", path);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + paddingY);

		ImGui::EndGroup();

		if (disabled)
			ImGui::EndDisabled();

		ImRect itemRect(cursorScreenPos, cursorScreenPos + ImVec2(itemWidth, ImGui::GetCursorPosY() - cursorPos.y));
		bool hovered, held;
		bool isDoubleClicked = ImGui::ButtonBehavior(itemRect, ImGui::GetID(name), &hovered, &held, ImGuiButtonFlags_PressedOnDoubleClick);

		if (hovered)
		{
			const ImU32 color = ImGui::IsMouseDown(0) ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 30);
			ImGui::GetWindowDrawList()->AddRectFilled(itemRect.Min, itemRect.Max, color, 5.0f);
		}
		return isDoubleClicked;
	}

	void Vec3Control(const std::string& str_id, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth)
	{
		ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 0);

		float lineHeight = ImGui::GetTextLineHeight() + 2.0f * ImGui::GetStyle().FramePadding.y;
		ImVec2 buttonSize = { 5.0f, lineHeight };

		ImGui::PushID(str_id.c_str());
		ImGui::PushMultiItemsWidths(3, ceil(availWidth + 7.0f - 3 * buttonSize.x));

		ScopedStyleVariable itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		{
			ScopedStyleColor button(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });

			if (ImGui::Button("##X_Button", buttonSize))
				value.x = defaultValue;
		}

		ImGui::SameLine();
		ImGui::DragFloat("##X", &value.x, dragSpeed, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		{
			ScopedStyleColor button(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });

			if (ImGui::Button("##Y_Button", buttonSize))
				value.y = defaultValue;
		}

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &value.y, dragSpeed, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		{
			ScopedStyleColor button(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

			if (ImGui::Button("##Z_Button", buttonSize))
				value.z = defaultValue;
		}

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &value.z, dragSpeed, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopID();
	}

	//////////////////////////////////////////// Scoped UI Utilities ////////////////////////////////////////////

	ScopedStyleColor::ScopedStyleColor(ImGuiCol idx, ImVec4 col, bool condition)
		: m_Condition(condition)
	{
		if (m_Condition)
			ImGui::PushStyleColor(idx, col);
	}

	ScopedStyleColor::ScopedStyleColor(ImGuiCol idx, ImU32 col, bool condition)
		: m_Condition(condition)
	{
		if (m_Condition)
			ImGui::PushStyleColor(idx, col);
	}

	ScopedStyleColor::~ScopedStyleColor()
	{
		if (m_Condition)
			ImGui::PopStyleColor();
	}

	ScopedStyleVariable::ScopedStyleVariable(ImGuiStyleVar idx, ImVec2 value, bool condition)
		: m_Condition(condition)
	{
		if (m_Condition)
			ImGui::PushStyleVar(idx, value);
	}

	ScopedStyleVariable::ScopedStyleVariable(ImGuiCol idx, float value, bool condition)
		: m_Condition(condition)
	{
		if (m_Condition)
			ImGui::PushStyleVar(idx, value);
	}

	ScopedStyleVariable::~ScopedStyleVariable()
	{
		if (m_Condition)
			ImGui::PopStyleVar();
	}

} // namespace Flameberry::UI
