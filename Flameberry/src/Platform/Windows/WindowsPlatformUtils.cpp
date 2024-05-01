#include "Platform/PlatformUtils.h"

#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Core/Application.h"

namespace Flameberry {
    namespace Platform {

        void CreateMenuBar() {}

        void OpenInExplorerOrFinder(const char* path)
        {

        }

        std::string OpenFile(const char* filter)
        {
            OPENFILENAMEA ofn;
            CHAR szFile[260] = { 0 };
            CHAR currentDir[256] = { 0 };
            ZeroMemory(&ofn, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = glfwGetWin32Window(Application::Get().GetWindow().GetGLFWwindow());
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            if (GetCurrentDirectoryA(256, currentDir))
                ofn.lpstrInitialDir = currentDir;
            ofn.lpstrFilter = filter;
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

            if (GetOpenFileNameA(&ofn) == TRUE)
                return ofn.lpstrFile;

            return std::string();

        }

        std::string SaveFile(const char* filter)
        {
            OPENFILENAMEA ofn;
            CHAR szFile[260] = { 0 };
            CHAR currentDir[256] = { 0 };
            ZeroMemory(&ofn, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = glfwGetWin32Window(Application::Get().GetWindow().GetGLFWwindow());
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            if (GetCurrentDirectoryA(256, currentDir))
                ofn.lpstrInitialDir = currentDir;
            ofn.lpstrFilter = filter;
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

            // Sets the default extension by extracting it from the filter
            ofn.lpstrDefExt = strchr(filter, '\0') + 1;

            if (GetSaveFileNameA(&ofn) == TRUE)
                return ofn.lpstrFile;

            return std::string();
        }

    }

}