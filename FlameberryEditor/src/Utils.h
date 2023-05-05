#pragma once

#include <string>
#include <glm/glm.hpp>

#define FL_REMOVE_LABEL(imgui_widget) { ImGui::PushItemWidth(-1); imgui_widget; ImGui::PopItemWidth(); }

class Utils
{
public:
    static void DrawVec3Control(const std::string& label, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth);
    static bool ButtonCenteredOnLine(const char* label, float alignment = 0.5f);
};
