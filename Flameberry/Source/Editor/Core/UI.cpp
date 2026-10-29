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
		ScopedStyleVariable framePadding(ImGuiStyleVar_FramePadding, { 0, (height - ImGui::GetFontSize()) * 0.5f });
		ScopedStyleColor windowBg(ImGuiCol_WindowBg, Theme::TitlebarColor);
		ScopedStyleColor menuBarBg(ImGuiCol_MenuBarBg, Theme::TitlebarColor);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse
			| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;
		ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		bool isOpen = ImGui::BeginViewportSideBar("##Titlebar", mainViewport, ImGuiDir_Up, height, windowFlags);

		g_UIState.IsTitlebarMenuBarOpen = isOpen && ImGui::BeginMenuBar();
		if (g_UIState.IsTitlebarMenuBarOpen) {
			// gradient background for titlebar
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

	bool Splitter(
		bool split_vertically,
		float thickness,
		float* size1,
		float* size2,
		float min_size1,
		float min_size2,
		float splitter_long_axis_size
	) {
		using namespace ImGui;
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiID id = window->GetID("##Splitter");
		ImRect bb;
		bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
		bb.Max =
			bb.Min
			+ CalcItemSize(
				split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness),
				0.0f, 0.0f
			);
		return SplitterBehavior(
			bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f, 0.0f, 0xFF000000
		);
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
			// Only counts while being typed in, including the frame enter is pressed (which deactivates the input), as
			// navigation focus alone lands on the input box whenever its window gets focused
			isActive = ImGui::IsItemActive() || ImGui::IsItemDeactivated();
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
		ImGui::SetNextWindowPos(
			ImGui::FindBestWindowPosForPopupEx(
				anchorRect.GetBL(), size, &g_UIState.SelectionWidgetLastDir, outerRect, anchorRect,
				ImGuiPopupPositionPolicy_ComboBox
			)
		);
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
		g_UIState.IsSelectionWidgetSearchBoxFocused = InputBox(
			inputBoxLabel.c_str(), -1.0f, inputBuffer, ICON_LC_SEARCH " Search...",
			g_UIState.IsSelectionWidgetSearchBoxFocused
		);

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
				"Attribute_Key", ImGuiTableColumnFlags_WidthFixed, labelWidth != 0.0f ? labelWidth : g_UIState.LabelWidth
			);
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

	bool PrimaryButton(const char* label, const ImVec2& size) {
		ScopedStyleColor button(ImGuiCol_Button, Theme::AccentColor);
		ScopedStyleColor buttonHovered(ImGuiCol_ButtonHovered, Theme::AccentColorLight);
		ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, Theme::SelectionColorHovered);
		ScopedStyleColor text(ImGuiCol_Text, Theme::SelectionTextColor);
		return ImGui::Button(label, size);
	}

	bool ProjectRegistryEntryItem(const char* name, const char* path, bool missing) {
		constexpr float padding = 10.0f, badgeSize = 36.0f, rounding = 6.0f;
		const float width = ImGui::GetContentRegionAvail().x, height = badgeSize + 2.0f * padding;
		const ImVec2 pos = ImGui::GetCursorScreenPos();

		ImGui::PushID(path);
		const bool pressed =
			ImGui::InvisibleButton("##ProjectEntry", ImVec2(width, height), ImGuiButtonFlags_PressedOnDoubleClick);
		const bool hovered = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
		ImGui::PopID();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
			ImGui::SetTooltip(missing ? "This project no longer exists at its path" : "Double-click to open");

		auto toU32 = [](const ImVec4& color) {
			return ImGui::ColorConvertFloat4ToU32(color);
		};
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		if (hovered || held)
			drawList->AddRectFilled(
				pos, pos + ImVec2(width, height), toU32(held ? Theme::HeaderBgActive : Theme::HeaderBgHovered), rounding
			);

		// Badge with the project's initial
		const ImVec2 badgeMin = pos + ImVec2(padding, padding);
		drawList->AddRectFilled(
			badgeMin, badgeMin + ImVec2(badgeSize, badgeSize), toU32(missing ? Theme::HeaderBg : Theme::AccentColor),
			rounding
		);
		{
			const char initial[2] = { name[0] ? (char)toupper(name[0]) : '?', '\0' };
			ImGui::PushFont(nullptr, 18.0f);
			const ImVec2 initialSize = ImGui::CalcTextSize(initial);
			drawList->AddText(
				badgeMin + (ImVec2(badgeSize, badgeSize) - initialSize) * 0.5f,
				toU32(missing ? Theme::TextDisabled : Theme::SelectionTextColor), initial
			);
			ImGui::PopFont();
		}

		// Name and path, stacked and centered vertically next to the badge
		const float fontSize = ImGui::GetFontSize(), lineGap = 4.0f;
		const float textX = badgeMin.x + badgeSize + padding, textMaxX = pos.x + width - padding;
		const float nameY = pos.y + 0.5f * (height - 2.0f * fontSize - lineGap), pathY = nameY + fontSize + lineGap;

		drawList->AddText(ImVec2(textX, nameY), toU32(missing ? Theme::TextDisabled : Theme::TextColor), name);

		if (missing) {
			constexpr const char* tag = "Missing";
			const ImVec2 tagPadding(6.0f, 1.0f);
			const ImVec2 tagMin(textX + ImGui::CalcTextSize(name).x + 8.0f, nameY - tagPadding.y);
			const ImVec2 tagMax = tagMin + ImGui::CalcTextSize(tag) + 2.0f * tagPadding;
			const ImVec4 errorColor = Theme::ErrorColor;
			drawList->AddRectFilled(
				tagMin, tagMax, toU32(ImVec4(errorColor.x, errorColor.y, errorColor.z, 0.18f)), 0.5f * (tagMax.y - tagMin.y)
			);
			drawList->AddText(tagMin + tagPadding, toU32(errorColor), tag);
		}

		{
			// Long paths are cut off with an ellipsis instead of running past the item
			ScopedStyleColor textColor(ImGuiCol_Text, missing ? Theme::TextDisabled : Theme::TextMuted);
			ImGui::RenderTextEllipsis(
				drawList, ImVec2(textX, pathY), ImVec2(textMaxX, pathY + fontSize), textMaxX, path, nullptr, nullptr
			);
		}

		return pressed && !missing;
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
