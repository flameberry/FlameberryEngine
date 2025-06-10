#pragma once

#include "Renderer/Texture2D.h"
#include <chrono>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/color.h>

namespace Flameberry {

	enum class EditorLogLevel : uint8_t
	{
		Info = 1,
		Warn,
		Error,
	};

	struct EditorLogEntry
	{
		std::string Message;
		EditorLogLevel LogLevel;
		std::chrono::system_clock::time_point Timestamp;

		EditorLogEntry(const std::string& message, EditorLogLevel logLevel, const std::chrono::system_clock::time_point& timestamp)
			: Message(message), LogLevel(logLevel), Timestamp(timestamp) {}
	};

	class LogPanel
	{
	public:
		template <typename... Args>
		inline void AddInfo(const fmt::format_string<Args...>& message, Args&&... args)
		{
			m_Logs.emplace_back(fmt::format(message, std::forward<Args>(args)...), EditorLogLevel::Info, std::chrono::system_clock::now());
			m_ScrollToBottom = true;
		}

		template <typename... Args>
		inline void AddWarning(const fmt::format_string<Args...>& message, Args&&... args)
		{
			m_Logs.emplace_back(fmt::format(message, std::forward<Args>(args)...), EditorLogLevel::Warn, std::chrono::system_clock::now());
			m_ScrollToBottom = true;
		}

		template <typename... Args>
		inline void AddError(const fmt::format_string<Args...>& message, Args&&... args)
		{
			m_Logs.emplace_back(fmt::format(message, std::forward<Args>(args)...), EditorLogLevel::Error, std::chrono::system_clock::now());
			m_ScrollToBottom = true;
		}

		LogPanel();

		void OnUIRender();

	private:
		void DisplayToolbar();
		void DisplayLogEntry(const EditorLogEntry& logEntry);
		void ClearLog();

	private:
		std::vector<EditorLogEntry> m_Logs;
		bool m_AutoScroll = true,
			 m_ScrollToBottom = false,
			 m_IsSearchBoxFocused = false,
			 m_FilterInfo = true,
			 m_FilterWarning = true,
			 m_FilterError = true;

		std::string m_SearchInputBuffer;

		Ref<Texture2D> m_InfoIcon, m_WarningIcon, m_ErrorIcon;
	};

} // namespace Flameberry
