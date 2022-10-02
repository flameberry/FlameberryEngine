#pragma once

#include "flamelogger/flamelogger.h"

namespace Flameberry {
    struct Logger {
        static void Init(const char* name);
        static std::shared_ptr<flamelogger::FLLoggerInstance> CoreLogger;
    };
}

#ifdef FL_DEBUG

#define FL_LOGGER_INIT(name) Flameberry::Logger::Init(name)
#define FL_SET_LOG_LEVEL(level) Flameberry::Logger::CoreLogger->SetLogLevel(level)

#define FL_DO_ON_ASSERT(x, ...) {if(!(x)) {__VA_ARGS__;}}
#define FL_ASSERT(x, ...) FL_DO_ON_ASSERT(x, Flameberry::Logger::CoreLogger->log_assert(__FILE__, __LINE__, __VA_ARGS__), FL_DEBUGBREAK())
#define FL_BASIC_ASSERT(x) FL_DO_ON_ASSERT(x, Flameberry::Logger::CoreLogger->log_assert(__FILE__, __LINE__), FL_DEBUGBREAK())

#define FL_DO_IN_ORDER(...) __VA_ARGS__;

#define FL_TRACE(...) Flameberry::Logger::CoreLogger->trace(__VA_ARGS__)
#define FL_LOG(...) Flameberry::Logger::CoreLogger->log(__VA_ARGS__)
#define FL_INFO(...) Flameberry::Logger::CoreLogger->info(__VA_ARGS__)
#define FL_WARN(...) Flameberry::Logger::CoreLogger->warn(__VA_ARGS__)
#define FL_ERROR(...) Flameberry::Logger::CoreLogger->error(__VA_ARGS__)
#define FL_CRITICAL(...) Flameberry::Logger::CoreLogger->critical(__VA_ARGS__)

#elif defined(FL_RELEASE)

#define FL_LOGGER_INIT(project_name)
#define FL_SET_LOG_LEVEL(level)

#define FL_DO_ON_ASSERT(x, ...)
#define FL_ASSERT(x, ...) if (!(x));
#define FL_BASIC_ASSERT(x) if (!(x));

#define FL_TRACE(...)
#define FL_LOG(...)
#define FL_INFO(...)
#define FL_ERROR(...)
#define FL_WARN(...)

#endif