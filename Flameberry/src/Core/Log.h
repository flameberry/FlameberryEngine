#pragma once

#include <filesystem>

#include <fmt/format.h>
#include <fmt/color.h>

/// Xcode doesn't support integrated terminal output by default, so the escape characters for coloring output are printed out
/// as it is, which just creates a mess, so the macro `FBY_XCODE_PROJ` is defined by CMake, if the cmake generator is `Xcode`
/// and the macros are defined as `""`
#ifdef FBY_XCODE_PROJ
#define __FBY_LOGGER_STYLE_FLAG
#endif

// Make std::filesystem::path formattable
template <>
struct fmt::formatter<std::filesystem::path> : formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& ctx)
    {
        return formatter<std::string_view>::format(path.string(), ctx);
    }
};

namespace Flameberry {
    enum class LogLevel : uint8_t { TRACE = 0, LOG, INFO, WARNING, ERROR, CRITICAL };

    class Logger
    {
    public:
        inline static std::shared_ptr<Logger> GetCoreLogger() { return s_CoreLogger; }
        inline static void SetCoreLogger(const std::shared_ptr<Logger>& logger) { s_CoreLogger = logger; }
    public:
        /// Instance Name should be set at the beginning of the program,
        /// which will be used as a prefix to all the log messages during runtime
        Logger(const char* instanceName);
        inline void SetLogLevel(const LogLevel& logLevel) { m_CurrentLogLevel = logLevel; }

        template <typename... Args>
        static std::shared_ptr<Logger> Create(Args... args) { return std::make_shared<Logger>(std::forward<Args>(args)...); }

        template<typename T, typename... Args>
        void log(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::LOG)
                fmt::print(fmt::fg(fmt::terminal_color::cyan), "{}: {}\n", GetPrefix(LogLevel::LOG), fmt::format(message, args...));
        }

        template<typename T, typename... Args>
        void trace(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::TRACE)
                fmt::print(fmt::fg(fmt::terminal_color::bright_white), "{}: {}\n", GetPrefix(LogLevel::TRACE), fmt::format(message, args...));
        }

        template<typename T, typename... Args>
        void info(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::INFO)
                fmt::print(fmt::fg(fmt::terminal_color::green), "{}: {}\n", GetPrefix(LogLevel::INFO), fmt::format(message, args...));
        }

        template<typename T, typename... Args>
        void warn(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::WARNING)
                fmt::print(fmt::fg(fmt::terminal_color::yellow), "{}: {}\n", GetPrefix(LogLevel::WARNING), fmt::format(message, args...));
        }

        template<typename T, typename... Args>
        void error(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::ERROR)
                fmt::print(fmt::fg(fmt::terminal_color::red), "{}: {}\n", GetPrefix(LogLevel::ERROR), fmt::format(message, args...));
        }

        template<typename T, typename... Args>
        void critical(const T& message, const Args&... args)
        {
            if (m_CurrentLogLevel <= LogLevel::CRITICAL)
                fmt::print(fmt::fg(fmt::terminal_color::red) | fmt::bg(fmt::color::yellow), "{}: {}\n", GetPrefix(LogLevel::CRITICAL), fmt::format(message, args...));
        }

        template<typename T, typename... Args>
        void log_assert(const char* file, int line, const T& message, const Args&... args)
        {
            std::string msg = fmt::format(message, args...);
            fmt::print(fmt::fg(fmt::terminal_color::red), "{}[ASSERT] Assertion failed: {} (file: {}, line: {})", GetCurrentTimeString(), msg, file, line);
        }

        void log_assert(const char* file, int line)
        {
            fmt::print(fmt::fg(fmt::terminal_color::red), "{}[ASSERT] Assertion failed: (file: {}, line: {})", GetCurrentTimeString(), file, line);
        }
    private:
        std::string GetCurrentTimeString();
        std::string GetPrefix(const LogLevel& logLevel);
    private:
        /// The name which is used in prefix of the log message
        std::string m_InstanceName;
        LogLevel m_CurrentLogLevel;

        static std::shared_ptr<Logger> s_CoreLogger;
    };
}

#ifdef FBY_DEBUG
#define FBY_DO_ON_ASSERT(x, ...) {if(!(x)) {__VA_ARGS__;}}
#define FBY_ASSERT(x, ...) FBY_DO_ON_ASSERT(x, Flameberry::Logger::GetCoreLogger()->log_assert(__FILE__, __LINE__, __VA_ARGS__), FBY_DEBUGBREAK())
#define FBY_BASIC_ASSERT(x) FBY_DO_ON_ASSERT(x, Flameberry::Logger::GetCoreLogger()->log_assert(__FILE__, __LINE__), FBY_DEBUGBREAK())

#define FBY_DO_IN_ORDER(...) __VA_ARGS__;
#else
#define FBY_DO_ON_ASSERT(x, ...)
#define FBY_ASSERT(x, ...) if (!(x));
#define FBY_BASIC_ASSERT(x) if (!(x));
#endif

#if defined(FBY_DEBUG) || defined(FBY_RELEASE)
#define FBY_TRACE(...) Flameberry::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define FBY_LOG(...) Flameberry::Logger::GetCoreLogger()->log(__VA_ARGS__)
#define FBY_INFO(...) Flameberry::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define FBY_WARN(...) Flameberry::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define FBY_ERROR(...) Flameberry::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define FBY_CRITICAL(...) Flameberry::Logger::GetCoreLogger()->critical(__VA_ARGS__)
#endif
