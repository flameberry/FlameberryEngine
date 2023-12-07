#pragma once

#include <string>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <filesystem>

#include "Renderer/Texture2D.h"

#define FBY_PUSH_WIDTH_MAX(imgui_widget) { ImGui::PushItemWidth(-1); imgui_widget; ImGui::PopItemWidth(); }

namespace Flameberry {

    class UI
    {
    public:
        static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
        static void Vec3Control(const std::string& label, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth);
        static bool AlignedButton(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f), float alignment = 0.5f);
        static void InputBox(const char* label, const float width, char* inputBuffer, const uint32_t inputLength, const char* inputHint = (const char*)__null);

        // Returns full width and height of the group including text and file icon
        static ImVec2 ContentBrowserItem(const std::filesystem::path& filepath, float size, const std::shared_ptr<Texture2D>& thumbnail, bool keepExtension = false);
        static bool ProjectRegistryEntryItem(const char* name, const char* path);
    };

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
    };

}
