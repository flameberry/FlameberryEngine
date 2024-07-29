#pragma once

#include <string>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <filesystem>

#include "Renderer/Texture2D.h"

#define FBY_PUSH_WIDTH_MAX(imgui_widget) \
	{                                    \
		ImGui::PushItemWidth(-1);        \
		imgui_widget;                    \
		ImGui::PopItemWidth();           \
	}

namespace Flameberry {

	inline ImVec4 operator*(const ImVec4& a, const ImVec4& b)
	{
		return ImVec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
	}

	inline ImVec4 operator*(const ImVec4& a, float b)
	{
		return ImVec4(a.x * b, a.y * b, a.z * b, a.w * b);
	}

	inline ImVec2 operator+(const ImVec2& a, const ImVec2& b)
	{
		return ImVec2(a.x + b.x, a.y + b.y);
	}

	inline ImVec2 operator-(const ImVec2& a, const ImVec2& b)
	{
		return ImVec2(a.x - b.x, a.y - b.y);
	}

	inline ImVec2 operator*(const ImVec2& a, float b)
	{
		return ImVec2(a.x * b, a.y * b);
	}

	inline ImVec2 operator/(const ImVec2& a, float b)
	{
		return ImVec2(a.x / b, a.y / b);
	}

	/**
	 * Contains all the UI widgets used by Flameberry
	 */
	namespace UI {

		bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
		void Vec3Control(const std::string& label, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth);
		bool AlignedButton(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f), float alignment = 0.5f);
		void InputBox(const char* label, const float width, std::string* inputBuffer, const char* inputHint = (const char*)__null);

		void OpenSelectionWidget(const char* label);
		bool BeginSelectionWidget(const char* label, std::string* inputBuffer);
		bool SelectionWidgetElement(const char* label, bool isSelected);
		void EndSelectionWidget();

		bool BeginKeyValueTable(const char* label, ImGuiTableFlags tableFlags = 0, float labelWidth = 0.0f);
		void TableKeyElement(const char* label);
		void EndKeyValueTable();

		bool ProjectRegistryEntryItem(const char* name, const char* path, bool disabled = false);
		// Returns full width and height of the group including text and file icon
		bool ContentBrowserItem(const std::filesystem::path& filepath, float size, const Ref<Texture2D>& thumbnail, ImVec2& outItemSize, bool keepExtension = false);

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
