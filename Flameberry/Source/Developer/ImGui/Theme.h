#pragma once

#include <imgui/imgui.h>

namespace Flameberry {

	class Theme
	{
	public:
		static constexpr ImVec4 AccentColor = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);
		static constexpr ImVec4 AccentColorLight = ImVec4(254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f);
		static constexpr ImVec4 WindowBg = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		static constexpr ImVec4 WindowBgGrey = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		static constexpr ImVec4 TableBorder = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);
		static constexpr ImVec4 FrameBg = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);
		static constexpr ImVec4 WindowBorder = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);
		static constexpr ImVec4 FrameBorder = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
		static constexpr ImVec4 DarkThemeColor = ImVec4(41.0f / 255.0f, 41.0f / 255.0f, 41.0f / 255.0f, 1.0f);
		static constexpr ImVec4 DarkThemeColorDark = ImVec4(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
		static constexpr ImVec4 TitlebarGreenColor = ImVec4(18.0f / 255.0f, 185.0f / 255.0f, 30.0f / 255.0f, 75.0f / 255.0f);
		static constexpr ImVec4 TitlebarRedColor = ImVec4(185.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 75.0f / 255.0f);
		static constexpr ImVec4 TitlebarOrangeColor = ImVec4(185.0f / 255.0f, 88.0f / 255.0f, 30.0f / 255.0f, 75.0f / 255.0f);
	};

} // namespace Flameberry
