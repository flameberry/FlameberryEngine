#pragma once

#include <imgui/imgui.h>

namespace Flameberry {

	namespace ThemeUtils {

		constexpr ImVec4 Gray(float value, float alpha = 1.0f) {
			return ImVec4(value / 255.0f, value / 255.0f, value / 255.0f, alpha);
		}

		constexpr ImVec4 RGB(float r, float g, float b, float alpha = 1.0f) {
			return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, alpha);
		}

	} // namespace ThemeUtils

	class Theme {
	public:
		// Surfaces, darkest to lightest: titlebar/tab strips < lists < panels < headers < buttons
		static constexpr ImVec4 TitlebarColor = ThemeUtils::Gray(19);
		static constexpr ImVec4 ImGuiTitleBg = ThemeUtils::Gray(21);
		static constexpr ImVec4 WindowBgDark = ThemeUtils::Gray(27);
		static constexpr ImVec4 WindowBg = ThemeUtils::Gray(34);
		static constexpr ImVec4 PopupBg = ThemeUtils::Gray(30);
		static constexpr ImVec4 HeaderBg = ThemeUtils::Gray(44);
		static constexpr ImVec4 HeaderBgHovered = ThemeUtils::Gray(54);
		static constexpr ImVec4 HeaderBgActive = ThemeUtils::Gray(60);
		static constexpr ImVec4 ButtonBg = ThemeUtils::Gray(52);
		static constexpr ImVec4 ButtonBgHovered = ThemeUtils::Gray(66);
		static constexpr ImVec4 ButtonBgActive = ThemeUtils::Gray(42);

		// Input fields sit below the panel surface
		static constexpr ImVec4 FrameBg = ThemeUtils::Gray(20);
		static constexpr ImVec4 FrameBgHovered = ThemeUtils::Gray(26);
		static constexpr ImVec4 FrameBgActive = ThemeUtils::Gray(16);
		static constexpr ImVec4 FrameBorder = ThemeUtils::Gray(48);

		static constexpr ImVec4 WindowBorder = ThemeUtils::Gray(14);
		static constexpr ImVec4 TableBorder = ThemeUtils::Gray(24);
		static constexpr ImVec4 TableBorderLight = ThemeUtils::Gray(28);

		static constexpr ImVec4 DarkThemeColor = WindowBg;
		static constexpr ImVec4 DarkThemeColorDark = WindowBgDark;

		static constexpr ImVec4 TextColor = ThemeUtils::Gray(222);
		static constexpr ImVec4 TextDisabled = ThemeUtils::Gray(120);
		static constexpr ImVec4 TextMuted = ThemeUtils::Gray(150);

		static constexpr ImVec4 AccentColor = ImVec4(252.0f / 255, 221.0f / 255, 141.0f / 255, 1.0f);
		static constexpr ImVec4 AccentColorLight = ImVec4(254.0f / 255.0f, 231.0f / 255.0f, 177.0f / 255.0f, 1.0f);

		// Selected rows are filled with the accent color, so they use dark text
		static constexpr ImVec4 SelectionColor = AccentColor;
		static constexpr ImVec4 SelectionColorHovered = ThemeUtils::RGB(254, 211, 140);
		static constexpr ImVec4 SelectionColorActive = AccentColorLight;
		static constexpr ImVec4 SelectionTextColor = ThemeUtils::Gray(0);
		static constexpr ImVec4 SelectionTextMuted = ThemeUtils::Gray(75);

		static constexpr ImVec4 TitlebarGreenColor =
			ImVec4(18.0f / 255.0f, 185.0f / 255.0f, 30.0f / 255.0f, 100.0f / 255.0f);
		static constexpr ImVec4 TitlebarRedColor = ImVec4(185.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 100.0f / 255.0f);
		static constexpr ImVec4 TitlebarOrangeColor =
			ImVec4(185.0f / 255.0f, 88.0f / 255.0f, 30.0f / 255.0f, 100.0f / 255.0f);

		static constexpr ImVec4 InfoColor = ImVec4(73.0f / 255, 148.0f / 255, 236.0f / 255, 1.0f);
		static constexpr ImVec4 WarningColor = ImVec4(230.0f / 255, 219.0f / 255, 111.0f / 255, 1.0f);
		static constexpr ImVec4 ErrorColor = ImVec4(0.90, 0.25, 0.25, 1.0);

		// X/Y/Z axis colors for vector controls
		static constexpr ImVec4 AxisXColor = ThemeUtils::RGB(196, 58, 58);
		static constexpr ImVec4 AxisYColor = ThemeUtils::RGB(92, 160, 52);
		static constexpr ImVec4 AxisZColor = ThemeUtils::RGB(52, 104, 200);
	};

} // namespace Flameberry
