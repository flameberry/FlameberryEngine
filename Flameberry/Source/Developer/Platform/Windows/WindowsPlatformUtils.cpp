#include "Platform/PlatformUtils.h"

#include <Windows.h>
#include <commdlg.h>
#include <ShlObj.h>

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

		void TitlebarNative::CreateForGLFWwindow(GLFWwindow* window, float titleBarHeight)
		{
		}

		void TitlebarNative::InvalidateFrameAndContentFrameRect(GLFWwindow* window, float titleBarHeight)
		{
		}

		void TitlebarNative::SetPrimaryTitle(const std::string& title)
		{
		}

		void TitlebarNative::SetSecondaryTitle(const std::string& secondaryTitle)
		{
		}

		void TitlebarNative::SetGradient(const glm::vec4& color)
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

		std::string OpenFolder()
		{
			BROWSEINFOA bi;
			CHAR szFolder[MAX_PATH] = { 0 };
			CHAR currentDir[MAX_PATH] = { 0 };

			ZeroMemory(&bi, sizeof(BROWSEINFOA));

			// Get the current directory and set it as the initial directory
			if (GetCurrentDirectoryA(MAX_PATH, currentDir))
				bi.lpszTitle = "Select a Folder"; // Text displayed in the dialog box

			bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;							   // Flags for only directories and the new dialog style
			bi.lpfn = NULL;																	   // Callback function, unused here
			bi.hwndOwner = glfwGetWin32Window(Application::Get().GetWindow().GetGLFWwindow()); // Owner window
			bi.lpszTitle = "Select Folder";

			// Open the folder selection dialog
			LPITEMIDLIST lpItem = SHBrowseForFolderA(&bi);

			if (lpItem != nullptr)
			{
				// Get the folder path from the selection
				if (SHGetPathFromIDListA(lpItem, szFolder))
				{
					return std::string(szFolder);
				}
			}

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

	} // namespace Platform

} // namespace Flameberry