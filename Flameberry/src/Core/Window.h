#pragma once

#include <memory>
#include <GLFW/glfw3.h>
#include "Event.h"

namespace Flameberry {
    class Window
    {
    public:
        static std::shared_ptr<Window> Create(int width = 1280, int height = 720, const char* title = "Flameberry Engine");

        virtual GLFWwindow* GetGLFWwindow() const = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetImageIndex() const = 0;

        virtual bool IsRunning() = 0;
        virtual void SwapBuffers() = 0;
        virtual bool BeginFrame() = 0;

        virtual bool IsWindowResized() const = 0;
        virtual void ResetWindowResizedFlag() = 0;

        virtual void SetEventCallBack(const std::function<void(Event&)>& fn) = 0;
        virtual void Init() = 0;
        virtual void Destroy() = 0;
    };

}