#pragma once

#include <iostream>
#include <GLFW/glfw3.h>

#include "Core/Window.h"
#include "Core/Log.h"

#ifdef FL_USE_VULKAN_API
#include "Renderer/VulkanRenderer/VulkanRenderer.h"
#elif defined(FL_USE_OPENGL_API
#include "Application.h"
#endif

int main(int argc, char const* argv[])
{
#ifdef FL_DEBUG
    FL_LOGGER_INIT("FLAMEBERRY");
    FL_SET_LOG_LEVEL(flamelogger::LogLevel::INFO);
    FL_INFO("Initialized Logger!");
#endif

#ifdef FL_USE_OPENGL_API
    auto clientApp = Flameberry::Application::CreateClientApp();
    clientApp->Run();
#elif defined(FL_USE_VULKAN_API)

    Flameberry::Window window;

    Flameberry::VulkanRenderer::Init(window.GetGLFWwindow());

    while (window.IsRunning())
    {
        Flameberry::VulkanRenderer::RenderFrame();
        window.OnUpdate();
    }

    Flameberry::VulkanRenderer::CleanUp();
#endif
    return 0;
}
