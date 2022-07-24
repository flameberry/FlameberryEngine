#pragma once
#include "Window.h"
#include <memory>

namespace Flameberry {
    class Application
    {
    public:
        Application();
        ~Application();
        void Run();
    private:
        std::shared_ptr<Window> M_Window;
    };
}