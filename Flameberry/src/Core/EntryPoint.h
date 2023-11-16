#pragma once

#include "Core/Log.h"
#include "Application.h"

int main(int argc, char const* argv[])
{
    std::shared_ptr<Flameberry::Logger> logger = Flameberry::Logger::Create("FLAMEBERRY");
    Flameberry::Logger::SetCoreLogger(logger);
    logger->SetLogLevel(Flameberry::LogLevel::LOG);

    Flameberry::Application* clientApp = Flameberry::Application::CreateClientApp();
    clientApp->Run();
    delete clientApp;

    return 0;
}
