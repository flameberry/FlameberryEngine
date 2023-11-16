#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <fmt/format.h>

/// Xcode doesn't support integrated terminal output by default, so the escape characters for coloring output are printed out
/// as it is, which just creates a mess, so the macro `FL_XCODE_PROJ` is defined by CMake, if the cmake generator is `Xcode`
/// and the macros are defined as `""`
#ifdef FL_XCODE_PROJ

#define FL_COLOR_DEFAULT ""
#define FL_COLOR_RED ""
#define FL_COLOR_GREEN ""
#define FL_COLOR_BRIGHT_GREEN ""
#define FL_COLOR_YELLOW ""
#define FL_COLOR_BRIGHT_YELLOW ""
#define FL_COLOR_PURPLE ""
#define FL_COLOR_CYAN ""
#define FL_COLOR_WHITE ""

#define FL_BG_COLOR_RED ""

#else

#define FL_COLOR_DEFAULT "\e[0m"
#define FL_COLOR_RED "\e[0;31m"
#define FL_COLOR_GREEN "\e[0;32m"
#define FL_COLOR_BRIGHT_GREEN "\e[0;92m"
#define FL_COLOR_YELLOW "\e[0;33m"
#define FL_COLOR_BRIGHT_YELLOW "\e[0;93m"
#define FL_COLOR_CYAN "\e[0;36m"
#define FL_COLOR_PURPLE "\e[0;35m"
#define FL_COLOR_WHITE "\e[0;37m"

#define FL_BG_COLOR_RED "\e[0;41m"

// #define FL_COLOR_DEFAULT "\033[0m"
// #define FL_COLOR_RED "\033[31m"
// #define FL_COLOR_GREEN "\033[32m"
// #define FL_COLOR_YELLOW "\033[33m"
// #define FL_COLOR_PURPLE "\033[35m"
// #define FL_COLOR_CYAN "\033[36m"
// #define FL_COLOR_WHITE "\033[37m"
#endif

// Make std::filesystem::path formattable
template <>
struct fmt::formatter<std::filesystem::path>: formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& ctx)
    {
        return formatter<std::string_view>::format(path.string(), ctx);
    }
};

namespace Flameberry {
    /// This enum class is currently only used for adding log level prefix to all logger messages
    enum class LogLevel : uint8_t { TRACE = 0, LOG, INFO, WARNING, ERROR, CRITICAL };

    std::string get_current_time_string();

    class Logger
    {
    public:
        /// Instance Name should be set at the beginning of the program,
        /// which will be used as a prefix to all the log messages during runtime
        Logger(const char* instanceName);
        static std::shared_ptr<Logger> Create(const char* instanceName);
        void SetLogLevel(const LogLevel& logLevel);

        /// Logs the message in CYAN color in the terminal
        template<typename T, typename... Args>
        void log(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::LOG)
            {
                std::cout << FL_COLOR_CYAN << get_prefix(LogLevel::LOG) << fmt::format(message, args...) << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in WHITE color in the terminal
        template<typename T, typename... Args>
        void trace(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::TRACE)
            {
                std::cout << FL_COLOR_WHITE << get_prefix(LogLevel::TRACE) << fmt::format(message, args...) << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in GREEN color in the terminal
        template<typename T, typename... Args>
        void info(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::INFO)
            {
                std::cout << FL_COLOR_BRIGHT_GREEN << get_prefix(LogLevel::INFO) << fmt::format(message, args...) << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in YELLOW color in the terminal
        template<typename T, typename... Args>
        void warn(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::WARNING)
            {
                std::cout << FL_COLOR_BRIGHT_YELLOW << get_prefix(LogLevel::WARNING) << fmt::format(message, args...) << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in RED color in the terminal
        template<typename T, typename... Args>
        void error(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::ERROR)
            {
                std::cout << FL_COLOR_RED << get_prefix(LogLevel::ERROR) << fmt::format(message, args...) << FL_COLOR_DEFAULT << std::endl;
            }
        }

        /// Logs the message in RED color in the terminal
        template<typename T, typename... Args>
        void critical(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::CRITICAL)
            {
                std::cout << FL_BG_COLOR_RED << get_prefix(LogLevel::CRITICAL) << fmt::format(message, args...) << FL_COLOR_DEFAULT << std::endl;
            }
        }

        template<typename T, typename... Args>
        void log_assert(const char* file, int line, const T& message, const Args&... args)
        {
            std::string msg = fmt::format(message, args...);
            std::cout << FL_COLOR_RED << fmt::format("{}[ASSERT] Assertion failed: {} (file: {}, line: {})", get_current_time_string(), msg, file, line) << FL_COLOR_DEFAULT << std::endl;
        }

        void log_assert(const char* file, int line)
        {
            std::cout << FL_COLOR_RED << fmt::format("{}[ASSERT] Assertion failed, file: {}, line: {}", get_current_time_string(), file, line) << FL_COLOR_DEFAULT << std::endl;
        }
    private:
        /// Gets the prefix of the log message
        std::string get_prefix(const LogLevel& logLevel);
    private:
        /// The name which is used in prefix of the log message
        const char* m_InstanceName = "";
        LogLevel m_CurrentLogLevel;
    };
}
