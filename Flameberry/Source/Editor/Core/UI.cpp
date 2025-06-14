#include "Core/UI.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <IconFontCppHeaders/IconsLucide.h>
#include <fmt/format.h>

#include "Core/Log.h"
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

	void AlignedText(const char* text, float alignment)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 size = ImGui::CalcTextSize(text);

		const float width = size.x ? size.x : ImGui::CalcTextSize(text).x + style.FramePadding.x * 2.0f;
		const float avail = ImGui::GetContentRegionAvail().x;

		const float off = (avail - width) * alignment;
		if (off > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		ImGui::Text("%s", text);
	}

	bool InputBox(const char* label, const float width, std::string* inputBuffer, const char* inputHint, bool focused)
	{
		ScopedStyleColor buttonColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ScopedStyleColor buttonHoverColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
		ScopedStyleColor buttonActiveColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

		constexpr ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll;

		ImGui::PushItemWidth(width);
		// Get current cursor position and input box size
		bool isActive = false;
		ImVec2 inputPos = ImGui::GetCursorScreenPos();
		ImVec2 inputSize = ImVec2(width, ImGui::GetFrameHeight());
		{
			ScopedStyleColor borderColor(ImGuiCol_Border, ImGui::ColorConvertFloat4ToU32(Theme::AccentColor), focused);
			ScopedStyleVariable frameRounding(ImGuiStyleVar_FrameRounding, 4);
			ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 0.5f);

			// Allow overlap for the upcoming button
			ImGui::SetNextItemAllowOverlap();
			ImGui::InputTextWithHint(label, inputHint, inputBuffer, flags);
			isActive = ImGui::IsItemActive() || ImGui::IsItemFocused();
		}
		ImGui::PopItemWidth();

		// Only show the clear button if there is text
		if (!inputBuffer->empty())
		{
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::SetCursorScreenPos(ImVec2(inputPos.x + width - inputSize.y, inputPos.y));

			ImGui::Button(ICON_LC_X, ImVec2(inputSize.y, inputSize.y));
			if (ImGui::IsItemClicked())
			{
				inputBuffer->clear();
				ImGui::SetKeyboardFocusHere(-1);
			}
		}
		return isActive;
	}

	void OpenSelectionWidget(const char* label)
	{
		const std::string labelFmt = fmt::format("{}Popup", label);
		ImGui::OpenPopup(labelFmt.c_str());

		g_UIState.IsSelectionWidgetJustOpened = true;
	}

	bool BeginSelectionWidget(const char* label, const char* title, std::string* inputBuffer)
	{
		const std::string labelFmt = fmt::format("{}Popup", label);

		if (ImGui::BeginPopup(labelFmt.c_str()))
		{
			if (g_UIState.IsSelectionWidgetJustOpened)
			{
				ImGui::SetKeyboardFocusHere();
				g_UIState.IsSelectionWidgetJustOpened = false;
			}

			UI::AlignedText(title, 0.5f);
			{
				ScopedStyleColor borderColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f }, g_UIState.IsSelectionWidgetSearchBoxFocused);
				ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 1.0f, g_UIState.IsSelectionWidgetSearchBoxFocused);

				const std::string inputBoxLabel = fmt::format("{}SearchBar", label);
				g_UIState.IsSelectionWidgetSearchBoxFocused = InputBox(inputBoxLabel.c_str(), -1.0f, inputBuffer, ICON_LC_SEARCH " Search...");
			}
			g_UIState.HasSelectionWidgetListBoxBegun = ImGui::BeginListBox(labelFmt.c_str());

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

	// TODO: Add display information on Hover

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

	bool Vec3Control(const std::string& str_id, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth)
	{
		bool isEdited = false;
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
			{
				value.x = defaultValue;
				isEdited = true;
			}
		}

		ImGui::SameLine();
		ImGui::DragFloat("##X", &value.x, dragSpeed, 0.0f, 0.0f, "%.2f");
		isEdited |= ImGui::IsItemEdited();
		ImGui::PopItemWidth();
		ImGui::SameLine();

		{
			ScopedStyleColor button(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });

			if (ImGui::Button("##Y_Button", buttonSize))
			{
				value.y = defaultValue;
				isEdited = true;
			}
		}

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &value.y, dragSpeed, 0.0f, 0.0f, "%.2f");
		isEdited |= ImGui::IsItemEdited();
		ImGui::PopItemWidth();
		ImGui::SameLine();

		{
			ScopedStyleColor button(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

			if (ImGui::Button("##Z_Button", buttonSize))
			{
				value.z = defaultValue;
				isEdited = true;
			}
		}

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &value.z, dragSpeed, 0.0f, 0.0f, "%.2f");
		isEdited |= ImGui::IsItemEdited();
		ImGui::PopItemWidth();

		ImGui::PopID();

		return isEdited;
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
