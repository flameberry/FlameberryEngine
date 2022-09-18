#pragma once

#include "Application.h"
#include "Core/Log.h"

int main(int argc, char const* argv[])
{
#ifdef FL_DEBUG
    FL_LOGGER_INIT("FLAMEBERRY");
    FL_INFO("Initialized Logger!");
#endif

    auto clientApp = Flameberry::Application::CreateClientApp();
    clientApp->Run();
    return 0;
}
