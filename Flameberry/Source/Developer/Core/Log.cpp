#include "Log.h"

#include <fmt/chrono.h>

namespace Flameberry {

	Ref<Logger> Logger::s_CoreLogger;

	Logger::Logger(const char* instanceName)
		: m_CurrentLogLevel(LogLevel::Trace), m_InstanceName(instanceName)
	{
	}

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
			case LogLevel::Trace:
				logLevelStr = "TRACE";
				break;
			case LogLevel::Log:
				logLevelStr = "LOG";
				break;
			case LogLevel::Info:
				logLevelStr = "INFO";
				break;
			case LogLevel::Warning:
				logLevelStr = "WARNING";
				break;
			case LogLevel::Error:
				logLevelStr = "ERROR";
				break;
			case LogLevel::Critical:
				logLevelStr = "CRITICAL";
				break;
		}
		return fmt::format("{} [{}] {}", GetCurrentTimeString(), m_InstanceName, logLevelStr);
	}

} // namespace Flameberry
