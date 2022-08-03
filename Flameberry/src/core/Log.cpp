#include "Log.h"

namespace Flameberry {
    std::shared_ptr<flamelogger::FLInstance> Log::s_CoreLoggerInstance;

    void Log::Init(const char* name)
    {
        s_CoreLoggerInstance = flamelogger::FLInstance::Create(name);
        s_CoreLoggerInstance->SetLogLevel(flamelogger::LogLevel::TRACE);
    }
}