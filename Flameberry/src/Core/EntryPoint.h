#pragma once

#include "Core/Log.h"
#include "Application.h"

int main(int argc, char const* argv[])
{
    FL_LOGGER_INIT("FLAMEBERRY");
    FL_SET_LOG_LEVEL(Flameberry::LogLevel::TRACE);
    FL_INFO("Initialized Logger!");
    
    Flameberry::Application* clientApp = Flameberry::Application::CreateClientApp();
    clientApp->Run();
    delete clientApp;
    
    return 0;
}
