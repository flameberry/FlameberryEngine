#pragma once

#include "Application.h"

int main(int argc, char const* argv[])
{
    auto clientApp = Flameberry::Application::CreateClientApp();
    clientApp->Run();
    return 0;
}
