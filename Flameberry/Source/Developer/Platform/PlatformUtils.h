#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <string>

namespace Flameberry {

	namespace Platform {

		class TitlebarNative
		{
		public:
			static void CreateForGLFWwindow(GLFWwindow* window, float titleBarHeight);
			static void InvalidateFrameAndContentFrameRect(GLFWwindow* window,
				float titleBarHeight);
			static void SetPrimaryTitle(const std::string& title);
			static void SetSecondaryTitle(const std::string& secondaryTitle);
		};

		void CreateMenuBar();
		void SetNewSceneCallbackMenuBar(const std::function<void()>& callback);
		void SetSaveSceneCallbackMenuBar(const std::function<void()>& callback);
		void SetSaveSceneAsCallbackMenuBar(const std::function<void()>& callback);
		void SetOpenSceneCallbackMenuBar(const std::function<void()>& callback);

		void OpenInExplorerOrFinder(const char* path);

		// Currently filter only works for windows
		std::string OpenFile(const char* filter);
		std::string OpenFolder();
		std::string SaveFile(const char* filter);

	} // namespace Platform

} // namespace Flameberry
