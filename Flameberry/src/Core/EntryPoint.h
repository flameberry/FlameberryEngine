#pragma once

#include "Core/Log.h"
#include "Application.h"

int main(int argc, char const* argv[])
{
    FL_LOGGER_INIT("FLAMEBERRY");
    FL_SET_LOG_LEVEL(flamelogger::LogLevel::TRACE);

    FL_INFO("Initialized Logger!");
    
    auto clientApp = Flameberry::Application::CreateClientApp();
    clientApp->Run();
    return 0;
}
