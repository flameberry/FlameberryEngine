#pragma once

#include <memory>
#include <GLFW/glfw3.h>

namespace Flameberry {
    class Window
    {
    public:
        static std::shared_ptr<Window> Create(int width = 1280, int height = 720, const char* title = "Flameberry Engine");

        virtual GLFWwindow* GetGLFWwindow() const = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual bool IsRunning() = 0;
        virtual void OnUpdate() = 0;
        virtual void SetKeyCallBack(GLFWkeyfun keyFn) = 0;
    };
}