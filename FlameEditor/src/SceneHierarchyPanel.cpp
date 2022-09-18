#include "SceneHierarchyPanel.h"

SceneHierarchyPanel::SceneHierarchyPanel(Flameberry::Scene* scene)
    : m_Scene(scene)
{
}

void SceneHierarchyPanel::OnUIRender()
{
    ImGui::Begin("Scene Hierarchy");
    // Iterate placeholder objects (all the same data)
    for (const auto& entity : m_Scene->GetRegistry()->GetEntityVector())
    {
        if (entity.is_valid())
        {
            if (ImGui::TreeNodeEx("Entity", ImGuiTreeNodeFlags_OpenOnArrow))
            {
                if (ImGui::IsItemClicked())
                    FL_LOG("Selected Entity!");
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}
