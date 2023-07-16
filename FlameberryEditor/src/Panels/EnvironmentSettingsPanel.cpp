#include "EnvironmentSettingsPanel.h"

#include "../Utils.h"
#include <filesystem>

namespace Flameberry {
    EnvironmentSettingsPanel::EnvironmentSettingsPanel(const std::shared_ptr<Scene>& context)
        : m_Context(context)
    {
    }

    void EnvironmentSettingsPanel::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Environment");
        ImGui::PopStyleVar();

        if (ImGui::BeginTable("Environment_Attributes", 2, m_TableFlags))
        {
            ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            auto& environment = m_Context->m_Environment;

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Enable EnvMap");
            ImGui::TableNextColumn();

            FL_PUSH_WIDTH_MAX(ImGui::Checkbox("##Enable_EnvMap", &environment.EnableEnvironmentMap));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            if (environment.EnableEnvironmentMap)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Env Map");
                ImGui::TableNextColumn();

                ImGui::Button(
                    environment.EnvironmentMap ? environment.EnvironmentMap->FilePath.filename().c_str() : "Null",
                    ImVec2(-1.0f, 0.0f)
                );

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
                    {
                        const char* path = (const char*)payload->Data;
                        std::filesystem::path envPath{ path };
                        const std::string& ext = envPath.extension().string();

                        FL_INFO("Payload recieved: {0}, with extension {1}", path, ext);

                        if (std::filesystem::exists(envPath) && std::filesystem::is_regular_file(envPath) && (ext == ".hdr"))
                            environment.EnvironmentMap = AssetManager::TryGetOrLoadAsset<Texture2D>(envPath);
                        else
                            FL_WARN("Bad File given as Environment!");
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Clear Color");
            ImGui::TableNextColumn();

            FL_PUSH_WIDTH_MAX(ImGui::ColorEdit3("##Clear_Color", glm::value_ptr(environment.ClearColor)));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Directional");
            ImGui::TableNextColumn();
            float colWidth = ImGui::GetColumnWidth();
            Utils::DrawVec3Control("##Directional_Light", environment.DirLight.Direction, 0.0f, 0.01f, colWidth);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Color");
            ImGui::TableNextColumn();

            FL_PUSH_WIDTH_MAX(ImGui::ColorEdit3("##Color", glm::value_ptr(environment.DirLight.Color)));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Intensity");
            ImGui::TableNextColumn();
            FL_PUSH_WIDTH_MAX(ImGui::DragFloat("##Intensity", &environment.DirLight.Intensity, 0.01f));
            ImGui::EndTable();
        }
        ImGui::End();
    }
}
