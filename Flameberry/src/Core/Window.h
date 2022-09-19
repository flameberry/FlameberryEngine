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
        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

        bool IsRunning();
        void OnUpdate();
    private:
        void InitForOpenGL();
        void InitForVulkan();
    private:
        GLFWwindow* m_Window;
        uint32_t m_Width, m_Height;
        const char* m_Title;
    };
}