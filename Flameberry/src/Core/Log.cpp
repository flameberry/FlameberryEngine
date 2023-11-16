#include "Log.h"

namespace Flameberry {
    std::shared_ptr<Flameberry::Logger> Log::CoreLogger = nullptr;
    void Log::Init(const char* name)
    {
        CoreLogger = Flameberry::Logger::Create(name);
        CoreLogger->SetLogLevel(Flameberry::LogLevel::TRACE);
    }
}
