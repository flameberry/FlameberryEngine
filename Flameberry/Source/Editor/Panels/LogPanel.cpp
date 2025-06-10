#include "LogPanel.h"

#include <IconFontCppHeaders/IconsLucide.h>
#include <fmt/chrono.h>
#include <imgui.h>

#include "Core/Algorithm.h"
#include "Core/UI.h"
#include "ImGui/Theme.h"
#include "Renderer/Texture2D.h"

namespace Flameberry {

	namespace Utils {

		inline ImVec4 LogLevelToColor(EditorLogLevel logLevel)
		{
			switch (logLevel)
			{
				case EditorLogLevel::Info:
					return Theme::InfoColor;
				case EditorLogLevel::Warn:
					return Theme::WarningColor;
				case EditorLogLevel::Error:
					return Theme::ErrorColor;
			}
		}

		inline const char* LogLevelToString(EditorLogLevel logLevel)
		{
			switch (logLevel)
			{
				case EditorLogLevel::Info:
					return "Info";
				case EditorLogLevel::Warn:
					return "Warning";
				case EditorLogLevel::Error:
					return "Error";
			}
		}

	} // namespace Utils

	LogPanel::LogPanel()
		: m_InfoIcon(Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/info.png"))
		, m_WarningIcon(Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/warning.png"))
		, m_ErrorIcon(Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/error.png"))
	{
	}

	void LogPanel::OnUIRender()
	{
		ImGui::Begin("Logs");

		DisplayToolbar();

		if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -1), ImGuiChildFlags_NavFlattened))
		{
			constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV
				| ImGuiTableFlags_NoKeepColumnsVisible
				| ImGuiTableFlags_RowBg
				| ImGuiTableFlags_PadOuterX
				| ImGuiTableFlags_HighlightHoveredColumn
				| ImGuiTableFlags_ScrollY;

			if (ImGui::BeginTable("##LogTable", 3, tableFlags))
			{
				ImGui::TableSetupScrollFreeze(3, 1);
				ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80);
				ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed, 80);
				ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				{
					UI::ScopedStyleVariable padding(ImGuiStyleVar_CellPadding, ImVec2(0, ImGui::GetStyle().CellPadding.y * 0.75f));
					for (const auto& log : m_Logs)
					{
						if (!m_FilterInfo && log.LogLevel == EditorLogLevel::Info)
							continue;
						if (!m_FilterWarning && log.LogLevel == EditorLogLevel::Warn)
							continue;
						if (!m_FilterError && log.LogLevel == EditorLogLevel::Error)
							continue;

						if (m_SearchInputBuffer[0] != '\0')
						{
							const int index = Algorithm::KmpSearch(log.Message.c_str(), m_SearchInputBuffer.c_str(), true);
							if (index == -1)
								continue;
						}

						DisplayLogEntry(log);
					}
				}

				// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
				// Using a scrollbar or mouse-wheel will take away from the bottom edge.
				if (m_ScrollToBottom || (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
					ImGui::SetScrollHereY(1.0f);
				m_ScrollToBottom = false;
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
		ImGui::End();
	}

	void LogPanel::DisplayToolbar()
	{
		if (ImGui::Button("Clear"))
			ClearLog();

		ImGui::SameLine();

		if (ImGui::Button(ICON_LC_SETTINGS))
			ImGui::OpenPopup("Options");

		ImGui::SameLine();

		UI::InputBox("##ContentBrowserSearchBar", 150.0f, &m_SearchInputBuffer, ICON_LC_SEARCH " Search...", m_IsSearchBoxFocused);
		m_IsSearchBoxFocused = ImGui::IsItemFocused();

		ImGui::SameLine();

		// Calculate total width of buttons
		float buttonSize = ImGui::CalcTextSize(ICON_LC_TRIANGLE_ALERT).x * 1.5f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float buttonWidth = buttonSize * 3
			+ ImGui::GetStyle().FramePadding.x * 2 * 3
			+ spacing * 2; // 3 buttons = 2 spacings

		// Move to right
		float rightAlignX = ImGui::GetContentRegionAvail().x - buttonWidth;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rightAlignX);

		if (ImGui::ImageButton("##Info",
				(ImTextureID)m_InfoIcon->CreateOrGetDescriptorSet(),
				ImVec2(buttonSize, buttonSize),
				ImVec2(0, 0),
				ImVec2(1, 1),
				ImVec4(0, 0, 0, 0),
				m_FilterInfo ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f)))
		{
			m_FilterInfo = !m_FilterInfo;
		}

		ImGui::SameLine();

		if (ImGui::ImageButton("##Warning",
				(ImTextureID)m_WarningIcon->CreateOrGetDescriptorSet(),
				ImVec2(buttonSize, buttonSize),
				ImVec2(0, 0),
				ImVec2(1, 1),
				ImVec4(0, 0, 0, 0),
				m_FilterWarning ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f)))
		{
			m_FilterWarning = !m_FilterWarning;
		}

		ImGui::SameLine();
		if (ImGui::ImageButton("##Error",
				(ImTextureID)m_ErrorIcon->CreateOrGetDescriptorSet(),
				ImVec2(buttonSize, buttonSize),
				ImVec2(0, 0),
				ImVec2(1, 1),
				ImVec4(0, 0, 0, 0),
				m_FilterError ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f)))
		{
			m_FilterError = !m_FilterError;
		}

		if (ImGui::Button("Add 10 Info logs"))
			for (int i = 10; i >= 0; i--)
				AddInfo("Info Number: {}", i);
		ImGui::SameLine();
		if (ImGui::Button("Add 10 Warning logs"))
			for (int i = 10; i >= 0; i--)
				AddWarning("Warning Number: {}", i);
		ImGui::SameLine();
		if (ImGui::Button("Add 10 Error logs"))
			for (int i = 10; i >= 0; i--)
				AddError("Error Number: {}", i);

		ImGui::Separator();

		if (ImGui::BeginPopup("Options"))
		{
			ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
			ImGui::EndPopup();
		}
	}

	void LogPanel::DisplayLogEntry(const EditorLogEntry& logEntry)
	{
		constexpr float width = 4.0f;
		ImGui::TableNextRow();

		// Column 0: colored vertical bar
		ImGui::TableNextColumn();

		// Get position for the colored box
		ImVec2 min = ImGui::GetCursorScreenPos();
		ImVec2 max = ImVec2(min.x + width, min.y + ImGui::GetTextLineHeightWithSpacing());
		ImU32 color = ImGui::ColorConvertFloat4ToU32(Utils::LogLevelToColor(logEntry.LogLevel));

		// Draw the filled vertical color box
		ImGui::GetWindowDrawList()->AddRectFilled(min, max, color);

		// Add dummy item to keep row height consistent
		ImGui::Dummy(ImVec2(width, ImGui::GetTextLineHeightWithSpacing()));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Utils::LogLevelToString(logEntry.LogLevel));

		// Timestamp
		ImGui::TableNextColumn();
		const time_t in_time_t = std::chrono::system_clock::to_time_t(logEntry.Timestamp);
		const std::string timestamp = fmt::format("{:%H:%M:%S}", *std::localtime(&in_time_t));
		ImGui::Text("%s", timestamp.c_str());

		// Column 1: log message text
		ImGui::TableNextColumn();
		ImGui::Text("%s", logEntry.Message.c_str());
		ImGui::SetItemTooltip("%s", logEntry.Message.c_str());
	}

	void LogPanel::ClearLog()
	{
		m_Logs.clear();
	}

} // namespace Flameberry
