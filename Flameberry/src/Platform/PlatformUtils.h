#pragma once

#include <string>
#include <functional>
#include <GLFW/glfw3.h>

namespace Flameberry {
    namespace platform {
        void CreateMenuBar();
        void CreateCustomTitleBar();

        void SetSaveSceneCallbackMenuBar(const std::function<void()>& callback);
        void SetSaveSceneAsCallbackMenuBar(const std::function<void()>& callback);
        void SetOpenSceneCallbackMenuBar(const std::function<void()>& callback);

        void OpenInExplorerOrFinder(const char* path);
        std::string OpenFile();
        std::string SaveFile();
    }
}
