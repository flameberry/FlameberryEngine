#pragma once

#include <iostream>
#include <GLFW/glfw3.h>

// #include "Application.h"
#include "Core/Window.h"
#include "Core/Log.h"

#include "Renderer/VulkanRenderer/VulkanRenderer.h"

int main(int argc, char const* argv[])
{
#ifdef FL_DEBUG
    FL_LOGGER_INIT("FLAMEBERRY");
    FL_INFO("Initialized Logger!");
#endif

    // auto clientApp = Flameberry::Application::CreateClientApp();
    // clientApp->Run();

    Flameberry::Window window;

    Flameberry::VulkanRenderer::Init(window.GetGLFWwindow());

    while (window.IsRunning())
    {
        Flameberry::VulkanRenderer::RenderFrame();
        window.OnUpdate();
    }

    Flameberry::VulkanRenderer::CleanUp();

    // glfwInit();

    // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // GLFWwindow* window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);

    // uint32_t extensionCount = 0;
    // vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    // FL_LOG("{0} extensions supported", extensionCount);

    // while (!glfwWindowShouldClose(window))
    // {
    //     glfwPollEvents();
    // }

    // glfwDestroyWindow(window);

    // glfwTerminate();
    return 0;
}
