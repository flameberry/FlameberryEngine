#pragma once
#include "Application.h"

int main(int argc, char const* argv[])
{
    auto clientApp = Flameberry::Application::CreateClient();
    clientApp->Run();
    return 0;
}
