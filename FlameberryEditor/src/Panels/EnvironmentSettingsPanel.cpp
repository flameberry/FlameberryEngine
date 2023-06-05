#include "EnvironmentSettingsPanel.h"

#include "../Utils.h"

namespace Flameberry {
    EnvironmentSettingsPanel::EnvironmentSettingsPanel(const std::shared_ptr<Scene>& context)
        : m_Context(context)
    {
    }

    void EnvironmentSettingsPanel::OnUIRender()
    {
        ImGui::Begin("Environment");

        if (ImGui::BeginTable("Environment_Attributes", 2, m_TableFlags))
        {
            ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            auto& environment = m_Context->m_SceneData.ActiveEnvironmentMap;

            ImGui::Text("Clear Color");
            ImGui::TableNextColumn();

            FL_REMOVE_LABEL(ImGui::ColorEdit3("##Clear_Color", glm::value_ptr(environment.ClearColor)));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Enable Skybox");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##Enable_Skybox", &environment.EnableSkybox);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            if (environment.EnableSkybox)
            {
                ImGui::Text("Env Reflections");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##Environment_Reflections", &environment.Reflections);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

            }
            else
                environment.Reflections = false;

            ImGui::Text("Directional");
            ImGui::TableNextColumn();
            float colWidth = ImGui::GetColumnWidth();
            Utils::DrawVec3Control("##Directional_Light", environment.DirLight.Direction, 0.0f, 0.01f, colWidth);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Color");
            ImGui::TableNextColumn();

            FL_REMOVE_LABEL(ImGui::ColorEdit3("##Color", glm::value_ptr(environment.DirLight.Color)));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Intensity");
            ImGui::TableNextColumn();
            FL_REMOVE_LABEL(ImGui::DragFloat("##Intensity", &environment.DirLight.Intensity, 0.01f));
            ImGui::EndTable();
        }
        ImGui::End();
    }
}