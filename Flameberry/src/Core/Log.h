#pragma once

#include "flamelogger/flamelogger.h"

namespace Flameberry {
    class Log
    {
    public:
        static void Init(const char* name);
        static std::shared_ptr<flamelogger::FLInstance> GetCoreLoggerInstance() { return s_CoreLoggerInstance; }
    private:
        static std::shared_ptr<flamelogger::FLInstance> s_CoreLoggerInstance;
    };
}

#ifdef FL_DEBUG

namespace flDebug {
    template<typename T, typename... Args>
    static void fl_print_msg_on_assert(const char* file, int line, const T& message, const Args&... args)
    {
        std::string msg = flamelogger::format_string(message, args...);
        std::string assert_message = flamelogger::format_string("{0}[ASSERT] Assertion failed: {1} (file: {2}, line: {3})", flamelogger::get_current_time_string(), msg, file, line);
        std::cout << FL_COLOR_RED << assert_message << FL_COLOR_DEFAULT << std::endl;
    }

    static void fl_print_msg_on_assert(const char* file, int line)
    {
        std::string assert_message = flamelogger::format_string("{0}[ASSERT] Assertion failed, file: {1}, line: {2}", flamelogger::get_current_time_string(), file, line);
        std::cout << FL_COLOR_RED << assert_message << FL_COLOR_DEFAULT << std::endl;
    }
}

#define FL_LOGGER_INIT(name) Flameberry::Log::Init(name)
#define FL_SET_LOG_LEVEL(level) Flameberry::Log::GetCoreLoggerInstance()->SetLogLevel(level)

#define FL_DO_ON_ASSERT(x, ...) {if(!(x)) {__VA_ARGS__;}}
#define FL_ASSERT(x, ...) FL_DO_ON_ASSERT(x, flDebug::fl_print_msg_on_assert(__FILE__, __LINE__, __VA_ARGS__), FL_DEBUGBREAK())
#define FL_BASIC_ASSERT(x) FL_DO_ON_ASSERT(x, flDebug::fl_print_msg_on_assert(__FILE__, __LINE__), FL_DEBUGBREAK())

#define FL_DO_IN_ORDER(...) __VA_ARGS__;

#define FL_TRACE(...) Flameberry::Log::GetCoreLoggerInstance()->trace(__VA_ARGS__)
#define FL_LOG(...) Flameberry::Log::GetCoreLoggerInstance()->log(__VA_ARGS__)
#define FL_INFO(...) Flameberry::Log::GetCoreLoggerInstance()->info(__VA_ARGS__)
#define FL_WARN(...) Flameberry::Log::GetCoreLoggerInstance()->warn(__VA_ARGS__)
#define FL_ERROR(...) Flameberry::Log::GetCoreLoggerInstance()->error(__VA_ARGS__)
#define FL_CRITICAL(...) Flameberry::Log::GetCoreLoggerInstance()->critical(__VA_ARGS__)

#elif defined(FL_RELEASE)

#define FL_LOGGER_INIT(project_name)

#define FL_DO_ON_ASSERT(x, ...)
#define FL_ASSERT(x, ...) if (!(x));
#define FL_BASIC_ASSERT(x) if (!(x));

#define FL_TRACE(...)
#define FL_LOG(...)
#define FL_INFO(...)
#define FL_ERROR(...)
#define FL_WARN(...)

#endif