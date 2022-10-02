#include "Log.h"

namespace Flameberry {
    std::shared_ptr<flamelogger::FLLoggerInstance> Logger::CoreLogger = nullptr;
    void Logger::Init(const char* name)
    {
        CoreLogger = flamelogger::FLLoggerInstance::Create(name);
        CoreLogger->SetLogLevel(flamelogger::LogLevel::TRACE);
    }
}