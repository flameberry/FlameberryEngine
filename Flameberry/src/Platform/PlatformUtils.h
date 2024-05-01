#pragma once

#include <string>
#include <functional>
#include <GLFW/glfw3.h>

namespace Flameberry {
    namespace platform {
        void CreateMenuBar();
        void SetupWindowForCustomTitleBar(GLFWwindow* window, float titleBarHeight);
        void InvalidateTitleBarFrameAndContentFrameRect(GLFWwindow* window, float titleBarHeight);

        void SetNewSceneCallbackMenuBar(const std::function<void()>& callback);
        void SetSaveSceneCallbackMenuBar(const std::function<void()>& callback);
        void SetSaveSceneAsCallbackMenuBar(const std::function<void()>& callback);
        void SetOpenSceneCallbackMenuBar(const std::function<void()>& callback);

        void OpenInExplorerOrFinder(const char* path);

        // Currently filter only works for windows 
        std::string OpenFile(const char* filter);
        std::string OpenFolder();
        std::string SaveFile(const char* filter);
    }
}
