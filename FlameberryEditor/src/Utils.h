#pragma once

#include <string>
#include <glm/glm.hpp>

#define FL_PUSH_WIDTH_MAX(imgui_widget) { ImGui::PushItemWidth(-1); imgui_widget; ImGui::PopItemWidth(); }

class Utils
{
public:
    static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
    static void DrawVec3Control(const std::string& label, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth);
    static bool ButtonCenteredOnLine(const char* label, float alignment = 0.5f);
};
