#include "Log.h"

#include <fmt/chrono.h>

namespace Flameberry {
	Ref<Logger> Logger::s_CoreLogger;

	Logger::Logger(const char* instanceName)
		: m_CurrentLogLevel(LogLevel::TRACE), m_InstanceName(instanceName) {}

	std::string Logger::GetCurrentTimeString()
	{
		std::time_t t = std::time(nullptr);
		return fmt::format("[{:%H:%M:%S}]", fmt::localtime(t));
	}

	std::string Logger::GetPrefix(const LogLevel& level)
	{
		std::string logLevelStr;
		switch (level)
		{
			case LogLevel::TRACE:
				logLevelStr = "TRACE";
				break;
			case LogLevel::LOG:
				logLevelStr = "LOG";
				break;
			case LogLevel::INFO:
				logLevelStr = "INFO";
				break;
			case LogLevel::WARNING:
				logLevelStr = "WARNING";
				break;
			case LogLevel::ERROR:
				logLevelStr = "ERROR";
				break;
			case LogLevel::CRITICAL:
				logLevelStr = "CRITICAL";
				break;
		}
		return fmt::format("{} [{}] {}", GetCurrentTimeString(), m_InstanceName,
			logLevelStr);
	}
} // namespace Flameberry
