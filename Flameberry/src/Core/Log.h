#pragma once

#include "flamelogger/Logger.h"

namespace Flameberry {
    struct Log {
        static void Init(const char* name);
        static std::shared_ptr<Flameberry::Logger> CoreLogger;
    };
}

#ifdef FL_DEBUG
#define FL_DO_ON_ASSERT(x, ...) {if(!(x)) {__VA_ARGS__;}}
#define FL_ASSERT(x, ...) FL_DO_ON_ASSERT(x, Flameberry::Log::CoreLogger->log_assert(__FILE__, __LINE__, __VA_ARGS__), FL_DEBUGBREAK())
#define FL_BASIC_ASSERT(x) FL_DO_ON_ASSERT(x, Flameberry::Log::CoreLogger->log_assert(__FILE__, __LINE__), FL_DEBUGBREAK())

#define FL_DO_IN_ORDER(...) __VA_ARGS__;
#else
#define FL_DO_ON_ASSERT(x, ...)
#define FL_ASSERT(x, ...) if (!(x));
#define FL_BASIC_ASSERT(x) if (!(x));
#endif

#if defined(FL_DEBUG) || defined(FL_RELEASE)

#define FL_LOGGER_INIT(name) Flameberry::Log::Init(name)
#define FL_SET_LOG_LEVEL(level) Flameberry::Log::CoreLogger->SetLogLevel(level)

#define FL_TRACE(...) Flameberry::Log::CoreLogger->trace(__VA_ARGS__)
#define FL_LOG(...) Flameberry::Log::CoreLogger->log(__VA_ARGS__)
#define FL_INFO(...) Flameberry::Log::CoreLogger->info(__VA_ARGS__)
#define FL_WARN(...) Flameberry::Log::CoreLogger->warn(__VA_ARGS__)
#define FL_ERROR(...) Flameberry::Log::CoreLogger->error(__VA_ARGS__)
#define FL_CRITICAL(...) Flameberry::Log::CoreLogger->critical(__VA_ARGS__)

#else

#define FL_Log_INIT(project_name)
#define FL_SET_LOG_LEVEL(level)

#define FL_TRACE(...)
#define FL_LOG(...)
#define FL_INFO(...)
#define FL_ERROR(...)
#define FL_WARN(...)

#endif
