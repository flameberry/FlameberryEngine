#pragma once

#include "Core/Window.h"

namespace Flameberry {
    class OpenGLWindow: public Window
    {
    public:
        OpenGLWindow(int width = 1280, int height = 720, const char* title = "Flameberry Engine");
        ~OpenGLWindow();

        GLFWwindow* GetGLFWwindow() const  override { return m_Window; }
        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }

        bool IsRunning() override { return !glfwWindowShouldClose(m_Window); }
        void OnUpdate() override;

        void SetEventCallBack(const std::function<void(Event&)>& fn) override;
    private:
        GLFWwindow* m_Window;
        uint32_t m_Width, m_Height;
        const char* m_Title;

        std::function<void(Event&)> m_EventCallBack;
    };
}