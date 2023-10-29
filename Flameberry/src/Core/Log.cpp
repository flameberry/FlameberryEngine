#include "Log.h"

namespace Flameberry {
    std::shared_ptr<flamelogger::Logger> Logger::CoreLogger = nullptr;
    void Logger::Init(const char* name)
    {
        CoreLogger = flamelogger::Logger::Create(name);
        CoreLogger->SetLogLevel(flamelogger::LogLevel::TRACE);
    }
}