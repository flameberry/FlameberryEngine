#pragma once

#include <string>
#include <functional>
#include <GLFW/glfw3.h>

namespace Flameberry {
    namespace platform {
        void DrawNativeMacOSMenuBar();

        void SetSaveSceneCallbackMenuBar(const std::function<void()>& callback);
        void SetSaveSceneAsCallbackMenuBar(const std::function<void()>& callback);
        void SetOpenSceneCallbackMenuBar(const std::function<void()>& callback);

        void OpenInFinder(const char* path);
        std::string OpenDialog();
        std::string SaveDialog();
    }
}
