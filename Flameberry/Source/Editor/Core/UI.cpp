#include "Core/UI.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <IconFontCppHeaders/IconsLucide.h>
#include <fmt/format.h>

#include "Core/Algorithm.h"
#include "ImGui/Theme.h"

namespace Flameberry::UI {

	struct UIState {
		// Fixed Layout/Style Properties
		static constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH
			| ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_PadOuterX;

		static constexpr float LabelWidth = 100.0f;

		// Selection Widget
		static constexpr float SelectionWidgetWidth = 280.0f;
		static constexpr int SelectionWidgetMaxVisibleRows = 10;

		ImRect SelectionWidgetAnchor;
		ImGuiDir SelectionWidgetLastDir = ImGuiDir_None;
		float SelectionWidgetHeight = 300.0f;
		const std::string* SelectionWidgetSearchBuffer = nullptr;
		int SelectionWidgetVisibleElementCount = 0;
		bool IsSelectionWidgetJustOpened = false, IsSelectionWidgetFirstFrame = false,
			 IsSelectionWidgetSearchBoxFocused = false, IsSelectionWidgetEnterPressed = false;

		// Titlebar
		bool IsTitlebarMenuBarOpen = false;
	};

	static UIState g_UIState;

	bool BeginTitlebar(float height, ImVec4 color) {
		ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ScopedStyleColor windowBg(ImGuiCol_WindowBg, Theme::TitlebarColor);
		ScopedStyleColor menuBarBg(ImGuiCol_MenuBarBg, Theme::TitlebarColor);

		// The titlebar widgets are hosted in a menu bar, so that menus open downwards like in a regular menu bar.
		// This frame padding makes the menu bar span the full titlebar height and vertically centers the widgets in it,
		// and is only kept while the menu bar begins, so that it doesn't affect the widgets and menus themselves
		ScopedStyleVariable framePadding(ImGuiStyleVar_FramePadding, { 0, (height - ImGui::GetFontSize()) * 0.5f });

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse
			| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;
		ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		bool isOpen = ImGui::BeginViewportSideBar("##Titlebar", mainViewport, ImGuiDir_Up, height, windowFlags);

		g_UIState.IsTitlebarMenuBarOpen = isOpen && ImGui::BeginMenuBar();
		if (g_UIState.IsTitlebarMenuBarOpen) {
			// gradient background for titlebar
			// This is drawn after the menu bar begins, as the window's own clip rect excludes the menu bar
			const ImU32 colu32 = ImGui::ColorConvertFloat4ToU32(color);
			ImVec2 pmin = ImGui::GetWindowPos(), pmax = pmin + ImVec2(ImGui::GetWindowSize().x / 2.5f, height);
			ImGui::GetWindowDrawList()->AddRectFilledMultiColor(pmin, pmax, colu32, 0x0, 0x0, colu32);

			// Avoid traffic lights on MacOS
			ImGui::Dummy({ 75.0f - ImGui::GetCursorPosX(), 0.0f });
		}
		return g_UIState.IsTitlebarMenuBarOpen;
	}

	void EndTitlebar() {
		if (g_UIState.IsTitlebarMenuBarOpen) ImGui::EndMenuBar();
		ImGui::End();
	}

	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2,
		float splitter_long_axis_size) {
		using namespace ImGui;
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiID id = window->GetID("##Splitter");
		ImRect bb;
		bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
		bb.Max = bb.Min
			+ CalcItemSize(
				split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness),
				0.0f, 0.0f);
		return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2,
			0.0f, 0.0f, 0xFF000000);
	}

	bool AlignedButton(const char* label, const ImVec2& size, float alignment) {
		const ImGuiStyle& style = ImGui::GetStyle();

		const float width = size.x ? size.x : ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
		const float avail = ImGui::GetContentRegionAvail().x;

		const float off = (avail - width) * alignment;
		if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		return ImGui::Button(label, size);
	}

	void AlignedText(const char* text, float alignment) {
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 size = ImGui::CalcTextSize(text);

		const float width = size.x ? size.x : ImGui::CalcTextSize(text).x + style.FramePadding.x * 2.0f;
		const float avail = ImGui::GetContentRegionAvail().x;

		const float off = (avail - width) * alignment;
		if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		ImGui::Text("%s", text);
	}

	bool InputBox(const char* label, float width, std::string* inputBuffer, const char* inputHint, bool focused) {
		ScopedStyleColor buttonColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ScopedStyleColor buttonHoverColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
		ScopedStyleColor buttonActiveColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

		constexpr ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll;

		ImGui::PushItemWidth(width);
		width = width == -1.0f ? ImGui::GetContentRegionAvail().x : width;
		// Get current cursor position and input box size
		bool isActive = false;
		ImGuiID inputId = 0;
		ImVec2 inputPos = ImGui::GetCursorScreenPos();
		ImVec2 inputSize = ImVec2(width, ImGui::GetFrameHeight());
		{
			// The input box draws its own focus border, so ImGui's navigation cursor would draw a second one around it
			ScopedStyleColor navCursor(ImGuiCol_NavCursor, ImVec4(0, 0, 0, 0));
			ScopedStyleColor borderColor(ImGuiCol_Border, ImGui::ColorConvertFloat4ToU32(Theme::AccentColor), focused);
			ScopedStyleVariable frameRounding(ImGuiStyleVar_FrameRounding, 4);
			ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 0.5f);

			// Allow overlap for the upcoming button
			ImGui::SetNextItemAllowOverlap();
			ImGui::InputTextWithHint(label, inputHint, inputBuffer, flags);
			inputId = ImGui::GetItemID();
			isActive = ImGui::IsItemActive() || ImGui::IsItemFocused();
		}
		ImGui::PopItemWidth();

		// Only show the clear button if there is text
		if (!inputBuffer->empty()) {
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::SetCursorScreenPos(ImVec2(inputPos.x + width - inputSize.y, inputPos.y));

			// Without a border, so that it doesn't cover the input box's own border
			ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 0.0f);
			if (ImGui::Button(ICON_LC_X, ImVec2(inputSize.y, inputSize.y))) {
				inputBuffer->clear();
				// Typing can carry on right after clearing
				ImGui::ActivateItemByID(inputId);
			}
		}
		return isActive;
	}

	void OpenSelectionWidget(const char* label) {
		const std::string labelFmt = fmt::format("{}Popup", label);
		ImGui::OpenPopup(labelFmt.c_str());

		// The widget is anchored to the item that opened it, which is usually the button that was just clicked
		g_UIState.SelectionWidgetAnchor = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		g_UIState.SelectionWidgetLastDir = ImGuiDir_None;
		g_UIState.IsSelectionWidgetJustOpened = true;
	}

	bool BeginSelectionWidget(const char* label, const char* title, std::string* inputBuffer) {
		const std::string labelFmt = fmt::format("{}Popup", label);
		if (!ImGui::IsPopupOpen(labelFmt.c_str())) return false;

		// Place the widget below the anchor, or above it when there isn't enough room, aligned to whichever of its
		// sides keeps the widget on screen. The height is the widget's height from the previous frame
		const ImGuiViewport* viewport = ImGui::GetWindowViewport();
		const ImRect outerRect(viewport->WorkPos, viewport->WorkPos + viewport->WorkSize);
		ImRect anchorRect = g_UIState.SelectionWidgetAnchor;
		anchorRect.Expand(ImVec2(0.0f, 4.0f));

		const ImVec2 size(g_UIState.SelectionWidgetWidth, g_UIState.SelectionWidgetHeight);
		ImGui::SetNextWindowPos(ImGui::FindBestWindowPosForPopupEx(anchorRect.GetBL(), size,
			&g_UIState.SelectionWidgetLastDir, outerRect, anchorRect, ImGuiPopupPositionPolicy_ComboBox));
		ImGui::SetNextWindowSize(ImVec2(g_UIState.SelectionWidgetWidth, 0.0f));

		{
			ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
			if (!ImGui::BeginPopup(labelFmt.c_str(), ImGuiWindowFlags_NoMove)) return false;
		}

		g_UIState.SelectionWidgetHeight = ImGui::GetWindowHeight();
		g_UIState.SelectionWidgetSearchBuffer = inputBuffer;
		g_UIState.SelectionWidgetVisibleElementCount = 0;
		g_UIState.IsSelectionWidgetFirstFrame = g_UIState.IsSelectionWidgetJustOpened;

		{
			ScopedStyleColor textColor(ImGuiCol_Text, Theme::TextMuted);
			ImGui::TextUnformatted(title);
		}

		if (g_UIState.IsSelectionWidgetJustOpened) {
			inputBuffer->clear();
			ImGui::SetKeyboardFocusHere();
			g_UIState.IsSelectionWidgetJustOpened = false;
		}

		const std::string inputBoxLabel = fmt::format("{}SearchBar", label);
		g_UIState.IsSelectionWidgetSearchBoxFocused = InputBox(inputBoxLabel.c_str(), -1.0f, inputBuffer,
			ICON_LC_SEARCH " Search...", g_UIState.IsSelectionWidgetSearchBoxFocused);

		// Pressing enter in the search box picks the first element that matches the search
		g_UIState.IsSelectionWidgetEnterPressed = g_UIState.IsSelectionWidgetSearchBoxFocused
			&& (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false));

		ImGui::Separator();

		// Rows are packed tighter than regular items, and the list scrolls once it is taller than the max visible rows
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 2.0f));
		const float maxListHeight =
			g_UIState.SelectionWidgetMaxVisibleRows * (ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y);
		ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, maxListHeight));
		ImGui::BeginChild("##SelectionWidgetList", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY);

		return true;
	}

	bool SelectionWidgetElement(const char* label, bool isSelected, const char* icon) {
		// Elements that don't match the search aren't displayed
		const std::string& search = *g_UIState.SelectionWidgetSearchBuffer;
		if (!search.empty() && Algorithm::KmpSearch(label, search.c_str(), true) == -1) return false;

		const bool isFirstElement = g_UIState.SelectionWidgetVisibleElementCount++ == 0;

		const ImGuiStyle& style = ImGui::GetStyle();
		const float rowHeight = ImGui::GetFrameHeight();
		const ImVec2 rowPos = ImGui::GetCursorScreenPos();

		bool pressed;
		{
			ScopedStyleColor header(ImGuiCol_Header, Theme::SelectionColor, isSelected);
			ScopedStyleColor headerHovered(ImGuiCol_HeaderHovered, Theme::SelectionColorHovered, isSelected);
			ScopedStyleColor headerActive(ImGuiCol_HeaderActive, Theme::SelectionColorActive, isSelected);

			ImGui::PushID(label);
			pressed = ImGui::Selectable("##SelectionWidgetElement", isSelected, 0, ImVec2(0.0f, rowHeight));
			ImGui::PopID();
		}
		pressed |= isFirstElement && g_UIState.IsSelectionWidgetEnterPressed;

		// The label is drawn separately from the selectable, so that it can be inset and follow the icon
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 textPos = rowPos + ImVec2(style.FramePadding.x, 0.5f * (rowHeight - ImGui::GetFontSize()));
		if (icon) {
			const ImVec4 iconColor = isSelected ? Theme::SelectionTextColor : Theme::TextMuted;
			drawList->AddText(textPos, ImGui::ColorConvertFloat4ToU32(iconColor), icon);
			textPos.x += ImGui::CalcTextSize(icon).x + style.ItemInnerSpacing.x;
		}
		const ImVec4 textColor = isSelected ? Theme::SelectionTextColor : Theme::TextColor;
		drawList->AddText(textPos, ImGui::ColorConvertFloat4ToU32(textColor), label);

		if (isSelected && g_UIState.IsSelectionWidgetFirstFrame) {
			ImGui::SetItemDefaultFocus();
			ImGui::SetScrollHereY();
		}

		if (pressed) ImGui::CloseCurrentPopup();
		return pressed;
	}

	void EndSelectionWidget() {
		if (g_UIState.SelectionWidgetVisibleElementCount == 0) {
			ImGui::Spacing();
			ImGui::TextDisabled("No matches");
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}

	bool BeginKeyValueTable(const char* label, ImGuiTableFlags tableFlags, float labelWidth) {
		if (ImGui::BeginTable(label, 2, tableFlags ? tableFlags : g_UIState.TableFlags)) {
			ImGui::TableSetupColumn(
				"Attribute_Key", ImGuiTableColumnFlags_WidthFixed, labelWidth != 0.0f ? labelWidth : g_UIState.LabelWidth);
			ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);
			return true;
		}
		return false;
	}

	void TableKeyElement(const char* label) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label);
		ImGui::TableNextColumn();
	}

	void EndKeyValueTable() {
		ImGui::EndTable();
	}

	// TODO: Add display information on Hover
	bool ProjectRegistryEntryItem(const char* name, const char* path, bool disabled) {
		constexpr float paddingX = 15.0f, paddingY = 5.0f, spacing = 10.0f;
		const float itemWidth = ImGui::GetContentRegionAvail().x;

		if (disabled) ImGui::BeginDisabled();

		// Anchor
		ImVec2 startPos = ImGui::GetCursorScreenPos();

		// --- Visual content ---
		ImGui::BeginGroup();

		ImGui::Indent(paddingX);
		ImGui::Dummy(ImVec2(0.0f, 2.0f * paddingY));
		ImGui::TextUnformatted(name);
		ImGui::Dummy(ImVec2(0.0f, spacing));
		ImGui::TextWrapped("%s", path);
		ImGui::Dummy(ImVec2(0.0f, paddingY));
		ImGui::Unindent(paddingX);

		ImGui::EndGroup();

		// Height is now known
		float height = ImGui::GetItemRectMax().y - startPos.y;

		// Rewind cursor INSIDE disabled/group scope
		ImGui::SetCursorScreenPos(startPos);

		// Claim the space BEFORE EndDisabled()
		bool pressed = ImGui::InvisibleButton(name, ImVec2(itemWidth, height), ImGuiButtonFlags_PressedOnDoubleClick);

		bool hovered = ImGui::IsItemHovered();

		if (hovered) {
			ImU32 color = ImGui::IsMouseDown(0) ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 30);

			ImGui::GetWindowDrawList()->AddRectFilled(startPos, startPos + ImVec2(itemWidth, height), color, 5.0f);
		}

		if (disabled) ImGui::EndDisabled();

		return pressed;
	}

	bool Vec3Control(const std::string& str_id, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth) {
		bool isEdited = false;
		ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 0);

		float lineHeight = ImGui::GetTextLineHeight() + 2.0f * ImGui::GetStyle().FramePadding.y;
		ImVec2 buttonSize = { 5.0f, lineHeight };

		ImGui::PushID(str_id.c_str());
		ImGui::PushMultiItemsWidths(3, ceil(availWidth + 7.0f - 3 * buttonSize.x));

		ScopedStyleVariable itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		{
			ScopedStyleColor button(ImGuiCol_Button, Theme::AxisXColor);
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, Theme::AxisXColor);
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, Theme::AxisXColor);

			if (ImGui::Button("##X_Button", buttonSize)) {
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
			ScopedStyleColor button(ImGuiCol_Button, Theme::AxisYColor);
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, Theme::AxisYColor);
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, Theme::AxisYColor);

			if (ImGui::Button("##Y_Button", buttonSize)) {
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
			ScopedStyleColor button(ImGuiCol_Button, Theme::AxisZColor);
			ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, Theme::AxisZColor);
			ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, Theme::AxisZColor);

			if (ImGui::Button("##Z_Button", buttonSize)) {
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

	ScopedStyleColor::ScopedStyleColor(ImGuiCol idx, ImVec4 col, bool condition) : m_Condition(condition) {
		if (m_Condition) ImGui::PushStyleColor(idx, col);
	}

	ScopedStyleColor::ScopedStyleColor(ImGuiCol idx, ImU32 col, bool condition) : m_Condition(condition) {
		if (m_Condition) ImGui::PushStyleColor(idx, col);
	}

	ScopedStyleColor::~ScopedStyleColor() {
		if (m_Condition) ImGui::PopStyleColor();
	}

	ScopedStyleVariable::ScopedStyleVariable(ImGuiStyleVar idx, ImVec2 value, bool condition) : m_Condition(condition) {
		if (m_Condition) ImGui::PushStyleVar(idx, value);
	}

	ScopedStyleVariable::ScopedStyleVariable(ImGuiCol idx, float value, bool condition) : m_Condition(condition) {
		if (m_Condition) ImGui::PushStyleVar(idx, value);
	}

	ScopedStyleVariable::~ScopedStyleVariable() {
		if (m_Condition) ImGui::PopStyleVar();
	}

} // namespace Flameberry::UI
