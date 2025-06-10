#pragma once

#include <string>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <filesystem>

#define FBY_PUSH_WIDTH_MAX(imgui_widget) \
	{                                    \
		ImGui::PushItemWidth(-1);        \
		imgui_widget;                    \
		ImGui::PopItemWidth();           \
	}

#define FBY_UI_TABLE_ELEMENT(name, element) \
	{                                       \
		UI::TableKeyElement(name);          \
		element;                            \
	}

#define FBY_UI_TABLE_ELEMENT_WIDTH_MAX(name, element) \
	{                                                 \
		UI::TableKeyElement(name);                    \
		ImGui::PushItemWidth(-1);                     \
		element;                                      \
		ImGui::PopItemWidth();                        \
	}

namespace Flameberry {

	/**
	 * Contains all the UI widgets used by Flameberry
	 */
	namespace UI {

		bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
		bool Vec3Control(const std::string& label, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth);
		bool AlignedButton(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f), float alignment = 0.5f);
		void AlignedText(const char* label, float alignment);
		void InputBox(const char* label, const float width, std::string* inputBuffer, const char* inputHint = (const char*)nullptr, bool focused = false);

		void OpenSelectionWidget(const char* label);
		bool BeginSelectionWidget(const char* label, const char* title, std::string* inputBuffer);
		bool SelectionWidgetElement(const char* label, bool isSelected);
		void EndSelectionWidget();

		bool BeginKeyValueTable(const char* label, ImGuiTableFlags tableFlags = 0, float labelWidth = 0.0f);
		void TableKeyElement(const char* label);
		void EndKeyValueTable();

		bool ProjectRegistryEntryItem(const char* name, const char* path, bool disabled = false);

		/**
		 *  Wrapper around ImGui::Push/PopStyleColor()
		 */
		class ScopedStyleColor
		{
		public:
			ScopedStyleColor(ImGuiCol idx, ImVec4 col, bool condition = true);
			ScopedStyleColor(ImGuiCol idx, ImU32 col, bool condition = true);
			~ScopedStyleColor();

		private:
			bool m_Condition;
		};

		/**
		 *  Wrapper around ImGui::Push/PopStyleVar()
		 */
		class ScopedStyleVariable
		{
		public:
			ScopedStyleVariable(ImGuiStyleVar idx, ImVec2 value, bool condition = true);
			ScopedStyleVariable(ImGuiCol idx, float value, bool condition = true);
			~ScopedStyleVariable();

		private:
			bool m_Condition;
		};

	} // namespace UI

} // namespace Flameberry
