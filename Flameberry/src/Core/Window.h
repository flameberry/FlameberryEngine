#pragma once
#include <memory>
#include <GLFW/glfw3.h>

namespace Flameberry {
    class Window
    {
    public:
        static std::shared_ptr<Window> Create(int width = 1280, int height = 720, const char* title = "Flameberry Engine");

        Window(int width = 1280, int height = 720, const char* title = "Flameberry Engine");
        ~Window();

        GLFWwindow* GetGLFWwindow() const { return m_Window; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

        bool IsRunning();
        void OnUpdate();
    private:
        GLFWwindow* m_Window;
        int m_Width, m_Height;
        const char* m_Title;
    };
}