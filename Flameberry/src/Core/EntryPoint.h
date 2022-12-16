#pragma once

#include "Core/Log.h"
#include "Application.h"

int main(int argc, char const* argv[])
{
#ifdef FL_DEBUG
    FL_LOGGER_INIT("FLAMEBERRY");
    FL_SET_LOG_LEVEL(flamelogger::LogLevel::LOG);

    FL_INFO("Initialized Logger!");
#endif
    auto clientApp = Flameberry::Application::CreateClientApp();
    clientApp->Run();
    return 0;
}
