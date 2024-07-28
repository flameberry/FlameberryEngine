#pragma once

#include <memory>
#include <GLFW/glfw3.h>

#include "Event.h"
#include "Core/Core.h"
#include "ImGui/Theme.h"

#ifdef FBY_DEBUG
	#define FBY_WINDOW_TITLE "Flameberry Engine [Debug]"
#elif defined(FBY_RELEASE)
	#define FBY_WINDOW_TITLE "Flameberry Engine [Release]"
#else
	#define FBY_WINDOW_TITLE "Flameberry Engine [Unknown]"
#endif

namespace Flameberry {

	struct WindowSpecification
	{
		int Width, Height;
		const char* Title = "";
		const char* SecondaryTitle = "";
		bool VSync;

		bool NativeTitlebar;
		glm::vec4 TitlebarGradientColor;
		static constexpr int TitlebarHeight = 34;

		WindowSpecification(int width = 1280, int height = 720, const char* title = FBY_WINDOW_TITLE)
			: Width(width)
			, Height(height)
			, Title(title)
			, NativeTitlebar(false)
			, TitlebarGradientColor(Theme::TitlebarGreenColor.x, Theme::TitlebarGreenColor.y, Theme::TitlebarGreenColor.z, Theme::TitlebarGreenColor.w)
		{
		}
	};

	class Window
	{
	public:
		static Ref<Window> Create(const WindowSpecification& specification = WindowSpecification());

		virtual GLFWwindow* GetGLFWwindow() const = 0;
		virtual const WindowSpecification& GetSpecification() const = 0;
		virtual uint32_t GetImageIndex() const = 0;

		virtual bool IsRunning() = 0;
		virtual void SwapBuffers() = 0;
		virtual bool BeginFrame() = 0;

		virtual void Resize() = 0;
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void SetEventCallBack(const std::function<void(Event&)>& fn) = 0;

		virtual void SetPosition(int xpos, int ypos) = 0;
		virtual void SetSize(int width, int height) = 0;
		virtual void SetTitle(const char* title) = 0;
		virtual void SetSecondaryTitle(const char* title) = 0;
		virtual void SetTitlebarGradient(const glm::vec4& color) = 0;
		virtual void MoveToCenter() = 0;
	};

} // namespace Flameberry
