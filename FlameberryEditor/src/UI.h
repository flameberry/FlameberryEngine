#pragma once

#include <string>
#include <glm/glm.hpp>
#include <imgui/imgui.h>

#define FL_PUSH_WIDTH_MAX(imgui_widget) { ImGui::PushItemWidth(-1); imgui_widget; ImGui::PopItemWidth(); }

namespace Flameberry {

    class UI
    {
    public:
        static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
        static void Vec3Control(const std::string& label, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth);
        static bool CenteredButton(const char* label, float alignment = 0.5f);
        static void SearchBar(const char* label, const float width, char* inputBuffer, const uint32_t inputLength, const char* inputHint = (const char*)__null);
    };

    class Theme
    {
    public:
        static constexpr ImVec4 AccentColor = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);
        static constexpr ImVec4 AccentColorLight = ImVec4(254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f);
    };

}
