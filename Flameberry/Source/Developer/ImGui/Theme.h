#pragma once

#include <imgui/imgui.h>

namespace Flameberry {

	class Theme
	{
	public:
		static constexpr ImVec4 AccentColor = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);
		static constexpr ImVec4 AccentColorLight = ImVec4(254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f);
		static constexpr ImVec4 WindowBgDark = ImVec4(27.0f / 255, 27.0f / 255, 27.0f / 255, 1.00f);
		static constexpr ImVec4 WindowBg = ImVec4(42.0f / 255, 42.0f / 255, 42.0f / 255, 1.00f);
		static constexpr ImVec4 TableBorder = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);
		static constexpr ImVec4 FrameBg = ImVec4(60.0f / 255, 60.0f / 255, 60.0f / 255, 1.0f);
		static constexpr ImVec4 WindowBorder = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);
		static constexpr ImVec4 FrameBorder = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
		static constexpr ImVec4 DarkThemeColor = ImVec4(41.0f / 255.0f, 41.0f / 255.0f, 41.0f / 255.0f, 1.0f);
		static constexpr ImVec4 DarkThemeColorDark = ImVec4(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
		static constexpr ImVec4 TitlebarGreenColor = ImVec4(18.0f / 255.0f, 185.0f / 255.0f, 30.0f / 255.0f, 100.0f / 255.0f);
		static constexpr ImVec4 TitlebarRedColor = ImVec4(185.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 100.0f / 255.0f);
		static constexpr ImVec4 TitlebarOrangeColor = ImVec4(185.0f / 255.0f, 88.0f / 255.0f, 30.0f / 255.0f, 100.0f / 255.0f);
		static constexpr ImVec4 TitlebarColor = ImVec4(21.0f / 255, 21.0f / 255, 21.0f / 255, 1.0f);
		static constexpr ImVec4 ImGuiTitleBg = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);
		static constexpr ImVec4 InfoColor = ImVec4(73.0f / 255, 148.0f / 255, 236.0f / 255, 1.0f);
		static constexpr ImVec4 WarningColor = ImVec4(230.0f / 255, 219.0f / 255, 111.0f / 255, 1.0f);
		static constexpr ImVec4 ErrorColor = ImVec4(0.90, 0.25, 0.25, 1.0);
	};

} // namespace Flameberry
